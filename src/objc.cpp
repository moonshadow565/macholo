#include "objc.hpp"

#include <dlfcn.h>
#include <objc/runtime.h>
#include <sys/mman.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "common.hpp"
#include "macho.hpp"

namespace objc::raw {
  union sel_ref_t {
    char const* name;
    SEL sel;
  };

  struct msg_ref_t {
    void* objc_msgSend_fixup;
    sel_ref_t sel;
  };

  template <typename T>
  struct list_t;

  struct method_t {
    static constexpr uint32_t LIST_FLAGS_MASK = 0b11;
    char const* name;
    char const* types;
    IMP impl;
  };

  struct ivar_t {
    static constexpr uint32_t LIST_FLAGS_MASK = 0;
    int32_t* offset;
    char const* name;
    char const* type;
    uint32_t alignment;
    uint32_t size;
  };

  struct property_t {
    static constexpr uint32_t LIST_FLAGS_MASK = 0;
    char const* name;
    char const* attributes;
  };

  struct protocol_t {
    static constexpr uint32_t LIST_FLAGS_MASK = 0;
    id isa;
    char const* name;
    list_t<protocol_t>* protocols;
    list_t<method_t>* methods;
    list_t<method_t>* static_methods;
    list_t<method_t>* opt_methods;
    list_t<method_t>* opt_static_methods;
    list_t<property_t>* properties;
    uint32_t size;
    uint32_t flags;
  };

  struct class_ro_t {
    uint32_t flags;
    uint32_t ivar_base_start;
    uint32_t ivar_base_size;
    uint32_t reserved;
    void* ivar_lyt;
    char* name;
    list_t<method_t>* base_methods;
    list_t<protocol_t>* base_protocols;
    list_t<ivar_t>* ivars;
    void* weak_ivar_lyt;
    list_t<property_t>* base_props;
  };

  struct class_t {
    class_t* isa;
    class_t* superclass;
    void* cache;
    void* vtable;
    uintptr_t bits;

    class_ro_t* data() const { return (class_ro_t*)(bits & ~uintptr_t{0b11}); }
    uintptr_t flags() const { return bits & uintptr_t{0b11}; }
  };

  struct category_t {
    char const* name;
    id cls;
    list_t<method_t>* methods;
    list_t<method_t>* static_methods;
    list_t<protocol_t>* protocols;
    list_t<property_t>* properties;
  };

  template <typename T>
  struct list_t {
    uint32_t size_and_flags;
    uint32_t count;
    T data[0];

    uint32_t entry_size() const { return size_and_flags & ~T::LIST_FLAGS_MASK; }

    uint32_t flags() const { return size_and_flags & T::LIST_FLAGS_MASK; }

    T* get(size_t i) { return (T*)(((char*)data) + i * entry_size()); }

    T const* get(size_t i) const { return (T*)(((char*)data) + i * entry_size()); }
  };

  template <>
  struct list_t<protocol_t> {
    uint64_t count;
    intptr_t ptrs[0];

    uint32_t entry_size() const { return sizeof(uintptr_t); }

    uint32_t flags() const { return 0; }

    protocol_t* get(size_t i, intptr_t slide) { return (protocol_t*)(ptrs[i] + slide); }

    protocol_t* get(size_t i, intptr_t slide) const { return (protocol_t*)(ptrs[i] + slide); }
  };

  struct cfstring_t {
    void* isa;
    int64_t info;
    char* data;
    uint32_t length;
    uint32_t pad;
  };

  // gnustep-2.x constant string, layout verified against clang -fobjc-runtime=gnustep-2.1
  // -fconstant-string-class=NSConstantString IR output: {isa, flags, length, size, hash, data}.
  // Same 32-byte size as Apple's cfstring_t, so literals convert in place.
  struct cfstring_gs_t {
    void* isa;        // ._OBJC_CLASS_NSConstantString
    uint32_t flags;   // clang emits 0
    uint32_t length;  // UTF-16 code units; == size for the ASCII literals we see
    uint32_t size;    // bytes
    uint32_t hash;    // 0 = computed lazily
    const char* data;
  };
  static_assert(sizeof(cfstring_gs_t) == sizeof(cfstring_t));

  struct Converter;

  static void dump_info(loader::Image* image);
}

static void objc::raw::dump_info(loader::Image* image) {
  for (auto p : image->find_section<protocol_t*>("__objc_protolist")) {
    printf("protocol %s {\n", p->name);
    if (p->protocols)
      for (size_t i = 0; i != p->protocols->count; ++i) {
        printf("\tprotocol %s;\n", p->protocols->get(i, image->slide)->name);
      }
    if (p->methods)
      for (size_t i = 0; i != p->methods->count; ++i) {
        printf("\tfn %s(%s);\n", p->methods->get(i)->name, p->methods->get(i)->types);
      }
    if (p->static_methods)
      for (size_t i = 0; i != p->static_methods->count; ++i) {
        printf("\tstatic fn %s(%s);\n", p->static_methods->get(i)->name, p->static_methods->get(i)->types);
      }
    if (p->opt_methods)
      for (size_t i = 0; i != p->opt_methods->count; ++i) {
        printf("\tfn %s(%s);\n", p->opt_methods->get(i)->name, p->opt_methods->get(i)->types);
      }
    if (p->opt_static_methods)
      for (size_t i = 0; i != p->opt_static_methods->count; ++i) {
        printf("\tstatic fn %s(%s);\n", p->opt_static_methods->get(i)->name, p->opt_static_methods->get(i)->types);
      }
    if (p->properties)
      for (size_t i = 0; i != p->properties->count; ++i) {
        printf("\tproperty %s { %s };\n", p->properties->get(i)->name, p->properties->get(i)->attributes);
      }
    printf("};\n\n");
  }

  for (auto c : image->find_section<class_t*>("__objc_classlist")) {
    auto const p = c->data();
    printf("class %s {\n", p->name);
    for (auto s : image->find_section<class_t*>("__objc_classlist"))
      if (s == c->isa) printf("\tsuper %s;", s->isa->data()->name);
    if (p->base_protocols)
      for (size_t i = 0; i != p->base_protocols->count; ++i) {
        printf("\tprotocol %s;\n", p->base_protocols->get(i, image->slide)->name);
      }
    if (p->base_methods)
      for (size_t i = 0; i != p->base_methods->count; ++i) {
        printf("\tfn %s(%s);\n", p->base_methods->get(i)->name, p->base_methods->get(i)->types);
      }
    if (p->base_props)
      for (size_t i = 0; i != p->base_props->count; ++i) {
        printf("\tproperty %s { %s };\n", p->base_props->get(i)->name, p->base_props->get(i)->attributes);
      }
    if (p->ivars)
      for (size_t i = 0; i != p->ivars->count; ++i) {
        printf("\tivar %s: %s;\n", p->ivars->get(i)->name, p->ivars->get(i)->type);
      }
    printf("};\n\n");
  }

  for (auto r : image->find_section<raw::sel_ref_t>("__objc_selrefs")) printf("SEL %s;\n", r.name);

  for (auto r : image->find_section<raw::msg_ref_t>("__objc_msgrefs")) printf("MSG %s;\n", r.sel.name);
}

struct objc::raw::Converter {
  Converter(intptr_t slide) : slide_(slide){};

  void import_classes(std::vector<macho::Binding> const& imports) {
    for (auto const& bind : imports) {
      if (bind.type != macho::Binding::TYPE_POINTER) continue;

      if (auto const p = std::string_view("_OBJC_CLASS_$_"); bind.name.starts_with(p)) {
        auto const class_name = bind.name.substr(p.size());
        auto const imp_class = objc_getClass(class_name.data());

        if (!imp_class) warn_unresolved("unresolved objc at %lx %s +%lx\n", bind.offset, bind.name.c_str(), bind.value);

        is_import_class_.insert((class_t*)imp_class);
        *(uintptr_t*)(bind.offset + slide_) = (uintptr_t)imp_class;
      }

      if (auto const p = std::string_view("_OBJC_METACLASS_$_"); bind.name.starts_with(p)) {
        auto const class_name = bind.name.substr(p.size());
        auto const imp_class = objc_getMetaClass(class_name.data());

        if (!imp_class) warn_unresolved("unresolved objc at %lx %s +%lx\n", bind.offset, bind.name.c_str(), bind.value);

        is_import_class_.insert((class_t*)imp_class);
        *(uintptr_t*)(bind.offset + slide_) = (uintptr_t)imp_class;
      }
    }
  }

  void convert_cfstrings(slice<cfstring_t> cfstrings) {
    for (auto& str : cfstrings) {
      convert_cfstring(&str);
    }
  }

  void convert_protocol_list(slice<uintptr_t> protocols) {
    for (auto& org_protocol : protocols) {
      lookup_prot_[(protocol_t*)org_protocol] = nullptr;
    }
    for (auto& org_protocol : protocols) {
      org_protocol = (uintptr_t)convert_protocol((protocol_t*)org_protocol);
    }
  }

  void convert_protocol_refs(slice<uintptr_t> protocol_refs) {
    for (auto& ref : protocol_refs) {
      if (auto const i = lookup_prot_.find((protocol_t*)ref); i != lookup_prot_.end()) {
        ref = (uintptr_t)i->second;
      }
    }
  }

  void convert_class_list(slice<uintptr_t> klasses) {
    for (auto& org_klass : klasses) {
      lookup_class_[(class_t*)org_klass] = nullptr;
      lookup_class_[((class_t*)org_klass)->isa] = nullptr;
    }
    for (auto& org_klass : klasses) {
      org_klass = (uintptr_t)convert_class((class_t*)org_klass);
    }
  }

  void convert_category_list(slice<uintptr_t> categories) {
    for (auto const org_category : categories) {
      convert_category((category_t*)org_category);
    }
  }

  void convert_class_refs(slice<uintptr_t> klass_refs) {
    for (auto& ref : klass_refs) {
      if (auto const i = lookup_class_.find((class_t*)ref); i != lookup_class_.end()) {
        ref = (uintptr_t)i->second;
      }
    }
  }

  void convert_sel_refs(slice<sel_ref_t> selrefs) {
    for (auto& ref : selrefs) {
      auto const org_sel = ref.name;
      auto const conv_sel = sel_getUid(org_sel);
      ref.sel = conv_sel;
    }
  }

  void convert_msg_refs(slice<msg_ref_t> msgrefs) {
    for (auto& ref : msgrefs) {
      auto const org_sel = ref.sel.name;
      auto const conv_sel = sel_getUid(org_sel);
      ref.sel.sel = conv_sel;
    }
  }

private:
  void convert_cfstring(cfstring_t* str) {
    auto tmp = *str;
    // Apple info 0x7C8 = immutable 8-bit null-terminated literal, which is every
    // literal seen so far. (The old check was assert_that(tmp.info = 0x7C8) -- an
    // assignment, so it never fired; keep behavior but tell the truth about it.)
    if (tmp.info != 0x7C8)
      fprintf(stderr, "[objc] cfstring with unexpected info %llx treated as 8-bit\n", (unsigned long long)tmp.info);
    cfstring_gs_t gnu{
        .isa = get_constant_string_isa(),
        .flags = 0,
        .length = (uint32_t)tmp.length,
        .size = (uint32_t)tmp.length,
        .hash = 0,
        .data = tmp.data,
    };
    memcpy(str, &gnu, sizeof(cfstring_t));
  }

  // gnustep-base's own constant-string class: guest literals become the same kind
  // of object our stub's compiled-in @"literals" are. This removed the last load-
  // time dependency on gnustep-corebase (NSCFString isa + CFRuntime hash patch).
  static void* get_constant_string_isa() {
    static void* isa = ::dlsym(nullptr, "._OBJC_CLASS_NSConstantString");
    assert_that(isa != nullptr);
    return isa;
  }

  Protocol* convert_protocol(protocol_t* protocol) {
    if (!protocol) return nullptr;
    if (auto const exist = objc_getProtocol(protocol->name)) return exist;
    if (auto const i = lookup_prot_.find(protocol); i != lookup_prot_.end()) {
      if (i->second != nullptr) return i->second;
    } else {
      panic("Unknown protocol!");
    }

    auto conv_protocol = objc_allocateProtocol(protocol->name);
    lookup_prot_[protocol] = conv_protocol;

    convert_protocol_subprotocols(conv_protocol, protocol->protocols);
    convert_protocol_methods<true, true>(conv_protocol, protocol->methods);
    convert_protocol_methods<true, false>(conv_protocol, protocol->static_methods);
    convert_protocol_methods<false, true>(conv_protocol, protocol->opt_methods);
    convert_protocol_methods<false, false>(conv_protocol, protocol->opt_static_methods);
    convert_protocol_properties(conv_protocol, protocol->properties);

    objc_registerProtocol(conv_protocol);
    return conv_protocol;
  }

  Class convert_class(raw::class_t* klass) {
    if (!klass) return nullptr;
    if (auto const i = lookup_class_.find(klass); i != lookup_class_.end()) {
      if (i->second != nullptr) return i->second;
    } else if (is_import_class_.contains(klass)) {
      return (Class)klass;
    } else {
      panic("unknown class!");
    }

    auto const super = convert_class(klass->superclass);
    assert_that(super != nullptr || klass->superclass == nullptr);

    auto const ro = klass->data();
    auto const ro_meta = klass->isa->data();

    auto const conv_klass = objc_allocateClassPair(super, ro->name, 0);
    auto const conv_isa = object_getClass(id(conv_klass));

    lookup_class_[klass] = conv_klass;
    lookup_class_[klass->isa] = conv_isa;

    log_objc("Converting class %s\n", ro->name);
    convert_class_methods(conv_klass, ro->base_methods);
    convert_class_methods(conv_isa, ro_meta->base_methods);
    convert_class_ivars(conv_klass, ro->ivars);
    convert_class_protocols(conv_klass, ro->base_protocols);
    convert_class_properties(conv_klass, ro->base_props);

    objc_registerClassPair(conv_klass);
    return conv_klass;
  }

  void convert_protocol_subprotocols(Protocol* conv_protocol, list_t<protocol_t>* subprotocols) {
    if (!subprotocols) return;
    for (size_t i = 0; i < subprotocols->count; ++i) {
      auto org_sub_protocol = subprotocols->get(i, slide_);
      assert_that(org_sub_protocol != nullptr);

      auto conv_sub_protocol = convert_protocol(org_sub_protocol);
      assert_that(conv_sub_protocol != nullptr);

      protocol_addProtocol(conv_protocol, conv_sub_protocol);
    }
  }

  template <bool IsRequired, bool IsInstance>
  void convert_protocol_methods(Protocol* conv_protocol, list_t<method_t>* methods) {
    if (!methods) return;
    for (size_t i = 0; i < methods->count; ++i) {
      auto org_method = methods->get(i);
      assert_that(org_method != nullptr);
      auto const types = convert_types(org_method->types);

      auto conv_method_name = sel_registerName(org_method->name);
      assert_that(conv_method_name);

      protocol_addMethodDescription(conv_protocol, conv_method_name, types.c_str(), IsRequired, IsInstance);
      log_objc("\tMethod %s(%s)\n", org_method->name, types.c_str());
    }
  }

  std::vector<objc_property_attribute_t> convert_attributes(std::string_view src) {
    std::vector<objc_property_attribute_t> results;
    while (!src.empty()) {
      std::string_view name;
      std::string_view value;
      if (src.front() == '"') {
        src = src.substr(1);
        auto const n = src.find('"');
        assert_that(n != std::string_view::npos);
        name = src.substr(n);
        src.substr(n + 1);
      } else {
        name = src.substr(0, 1);
        src = src.substr(1);
      }
      if (auto const n = src.find(','); n != std::string_view::npos) {
        value = src.substr(0, n);
        src = src.substr(n + 1);
      } else {
        value = src;
        src = src.substr(src.size());
      }
      results.push_back(objc_property_attribute_t{
          .name = strndup(name.data(), name.size()),
          .value = strndup(value.data(), value.size()),
      });
    }
    return results;
  }

  void convert_protocol_properties(Protocol* conv_protocol, list_t<property_t>* properties) {
    if (!properties) return;
    for (size_t i = 0; i != properties->count; ++i) {
      auto org_property = properties->get(i);
      auto conv_attrs = convert_attributes(org_property->attributes);
      protocol_addProperty(conv_protocol, org_property->name, conv_attrs.data(), conv_attrs.size(), true, true);
      log_objc("\tProperty %s(%s)\n", org_property->name, org_property->attributes);
    }
  }

  // Categories extend classes that already exist by the time we get here -- either a
  // host class the binary imports (patched in place by import_classes) or a local class
  // convert_class_list just registered. libobjc2 has no category-registration entry
  // point for foreign metadata, so attach the members directly to the target class.
  // class_replaceMethod (not class_addMethod) gives category semantics: add the method,
  // or override the class's own definition if it has one.
  void convert_category(category_t* category) {
    if (!category) return;

    auto const conv_klass = convert_class((class_t*)category->cls);
    if (!conv_klass) {
      warn_unresolved("unresolved objc category %s (target class missing)\n", category->name);
      return;
    }

    log_objc("Converting category %s(%s)\n", class_getName(conv_klass), category->name);
    add_category_methods(conv_klass, category->methods);
    add_category_methods(object_getClass((id)conv_klass), category->static_methods);
    convert_class_protocols(conv_klass, category->protocols);
    convert_class_properties(conv_klass, category->properties);
  }

  void add_category_methods(Class conv_klass, list_t<method_t>* methods) {
    if (!methods) return;
    for (size_t i = 0; i != methods->count; ++i) {
      auto const org_method = methods->get(i);
      auto const types = convert_types(org_method->types);
      auto const conv_sel = sel_registerTypedName_np(org_method->name, types.c_str());
      class_replaceMethod(conv_klass, conv_sel, org_method->impl, types.c_str());
      log_objc("\tMethod %s(%s)\n", org_method->name, types.c_str());
    }
  }

  void convert_class_methods(Class conv_klass, list_t<method_t>* methods) {
    if (!methods) return;
    for (size_t i = 0; i != methods->count; ++i) {
      auto const org_method = methods->get(i);
      auto const types = convert_types(org_method->types);
      auto const conv_sel = sel_registerTypedName_np(org_method->name, types.c_str());
      class_addMethod(conv_klass, conv_sel, org_method->impl, types.c_str());
      log_objc("\tMethod %s(%s)\n", org_method->name, types.c_str());
      // TODO detect load functions?
    }
  }

  void convert_class_ivars(Class conv_klass, list_t<ivar_t>* ivars) {
    struct objc_ivar_hack {
      const char* name;
      const char* type;
      int* offset;
      uint32_t size;
      uint32_t flags;
    };
    if (!ivars) return;
    for (size_t i = 0; i != ivars->count; ++i) {
      auto const org_ivar = ivars->get(i);
      // TODO: alingment apears to be in EXACTLY the format we want, darling thinks otherwise but to me darling implementation seems blatantly wrong...
      auto const ok = class_addIvar(conv_klass, org_ivar->name, org_ivar->size, org_ivar->alignment, org_ivar->type);
      assert_that(ok);

      auto const conv_ivar = (objc_ivar_hack*)class_getInstanceVariable(conv_klass, org_ivar->name);
      assert_that(conv_ivar != nullptr);
      // FIXME: ivar_getOffset is broken?
      // *(org_ivar->offset) = ivar_getOffset(conv_ivar);
      *(org_ivar->offset) = (int32_t)(intptr_t)conv_ivar->offset;
      log_objc("\tIvar %s[@%x+%x]\n", org_ivar->name, *org_ivar->offset, org_ivar->size);
    }
  }

  void convert_class_protocols(Class conv_klass, list_t<protocol_t>* protocols) {
    if (!protocols) return;
    for (size_t i = 0; i != protocols->count; ++i) {
      auto const org_prot = protocols->get(i, slide_);
      auto const conv_prot = convert_protocol(org_prot);
      assert_that(conv_prot != nullptr);

      class_addProtocol(conv_klass, conv_prot);
    }
  }

  void convert_class_properties(Class conv_klass, list_t<property_t>* properties) {
    if (!properties) return;
    for (size_t i = 0; i != properties->count; ++i) {
      auto org_property = properties->get(i);
      auto conv_attrs = convert_attributes(org_property->attributes);
      class_addProperty(conv_klass, org_property->name, conv_attrs.data(), conv_attrs.size());
      log_objc("\tProperty %s(%s)\n", org_property->name, org_property->attributes);
    }
  }

  std::string convert_types(std::string str) {
    static constexpr std::pair<std::string_view, std::string_view> ALIAS[] = {
        {"CGRect", "_NSRect"},
        {"CGPoint", "_NSPoint"},
        {"CGSize", "_NSSize"},
    };
    for (auto const [from, into] : ALIAS) {
      for (auto p = 0; (p = str.find(from, p)) != std::string::npos; p += into.size()) {
        str.replace(p, from.size(), into);
      }
    }
    return str;
  }

  intptr_t slide_;
  std::unordered_map<raw::protocol_t*, Protocol*> lookup_prot_;
  std::unordered_map<raw::class_t*, Class> lookup_class_;
  std::unordered_set<raw::class_t*> is_import_class_;
};

void objc::fixup_image(loader::Image* image) {
  // objc::raw::dump_info(image);

  objc::raw::Converter converter(image->slide);
  converter.import_classes(image->object.bind);
  converter.import_classes(image->object.bind_lazy);
  converter.convert_protocol_list(image->find_section<uintptr_t>("__objc_protolist"));
  converter.convert_class_list(image->find_section<uintptr_t>("__objc_classlist"));
  converter.convert_category_list(image->find_section<uintptr_t>("__objc_catlist"));
  converter.convert_cfstrings(image->find_section<raw::cfstring_t>("__cfstring"));

  converter.convert_protocol_refs(image->find_section<uintptr_t>("__objc_protorefs"));
  converter.convert_class_refs(image->find_section<uintptr_t>("__objc_classrefs"));
  converter.convert_class_refs(image->find_section<uintptr_t>("__objc_superrefs"));
  converter.convert_sel_refs(image->find_section<raw::sel_ref_t>("__objc_selrefs"));
  converter.convert_msg_refs(image->find_section<raw::msg_ref_t>("__objc_msgrefs"));
  // TODO: init ?
}

bool objc::is_objc_name(std::string_view name) {
  if (name == "___CFConstantStringClassReference") return true;
  if (name.starts_with("_OBJC_")) return true;
  return false;
}
