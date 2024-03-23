#include "ak.hpp"

#include <dirent.h>
#include <dlfcn.h>
// do not reorder
#include <alsa/global.h>
// do not reorder
#include <alsa/input.h>
// do not reorder
#include <alsa/output.h>
// do not reorder
#include <alsa/conf.h>
// do not reorder
#include <alsa/pcm.h>
// do not reorder

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <optional>
#include <thread>

#include "common.hpp"

namespace ak::raw {
  enum class AKRESULT {
    NotImplemented = 0,
    Success = 1,
    Fail = 2,
    InsufficientMemory = 0x34,
  };

  enum AkPluginType {
    AkPluginTypeSink = 0x7,
  };

  struct AkPluginInfo {
    uint64_t unk0;  // 0x7E30100000007
    uint8_t unk1;   // 0
  };

  struct AkChannelConfig {
    uint32_t num_channels : 8;
    uint32_t config_type : 4;
    uint32_t channel_mask : 20;
  };

  struct AkThreadProperties {
    int nPriority;
    size_t uStackSize;
    int uSchedPolicy;
    uint32_t dwAffinityMask;
  };

  struct AkPlatformInitSettings {
    AkThreadProperties threadLEngine;
    AkThreadProperties threadOutputMgr;
    AkThreadProperties threadBankManager;
    AkThreadProperties threadMonitor;
    float unk0;
    uint32_t unk1; // eAudioAPI ?
    uint32_t uSampleRate;
    uint16_t uNumRefillsInVoice;
    // uint16_t sampleType; ?
  };

  struct IAkPluginMemAlloc {
    virtual ~IAkPluginMemAlloc() = 0;
    virtual void* Malloc(size_t size, char const* file, uint32_t line) = 0;
    virtual void Free(void* pointer) = 0;
    virtual void* MallocAlign(size_t size, size_t align, char const* file, uint32_t line) = 0;
    virtual void* Realloc(void* pointer, size_t size, char const* file, uint32_t line) = 0;
    virtual void* ReallocAlign(void* pointer, size_t size, size_t align, char const* file, uint32_t line) = 0;
  };

  struct IAkGlobalPluginContext {
    virtual ~IAkGlobalPluginContext() = 0;
    virtual void* GetStreamMgr() const = 0;
    virtual uint16_t GetMaxBufferLength() const = 0;
    virtual bool IsRenderingOffline() const = 0;
    virtual uint32_t GetSampleRate() const = 0;
    virtual void todo_00();  // PostMonitorMessage
    virtual void todo_01();  // RegisterPlugin
    virtual void todo_02();  // RegisterCodec
    virtual void todo_03();  // RegisterGlobalCallback
    virtual void todo_04();  // UnregisterGlobalCallback
    virtual void todo_05();  // GetAllocator
    virtual void todo_06();  // SetRTPCValue
    virtual void todo_07();  // SendPluginCustomGameData
    virtual void todo_08();  // MixNinNChannels
    virtual void todo_09();  // Mix1inNChannels
    virtual void todo_0a();  // MixChannel
    virtual void todo_0b();  // ComputeAmbisonicsEncoding
    virtual void todo_0c();  // ComputeWeightedAmbisonicsDecodingFromSampledSphere
    virtual void todo_0d();  // GetAcousticTexture
    virtual void todo_0e();  // ComputeSphericalCoordinates
    virtual AkPlatformInitSettings* GetPlatformInitSettings();
  };

  struct IAkSinkPluginContext {
    virtual ~IAkSinkPluginContext() = 0;
    virtual IAkGlobalPluginContext* GlobalContext() const = 0;
    virtual void* GetGameObjectInfo() = 0;
    virtual AKRESULT GetOutputID(uint32_t* out_output_id, uint32_t* out_plugin_id) = 0;
    virtual void GetPluginMedia(uint32_t index, uint8_t** out_data, uint32_t* out_size) = 0;
    virtual void GetPluginCustomGameData(void** out_data, uint32_t* out_data_size) = 0;
    virtual AKRESULT PostMonitorData(void* data, uint32_t size) = 0;
    virtual bool CanPostMonitorData() = 0;
    virtual AKRESULT PostMonitorMessage(char const* message, int level) = 0;
    virtual float GetDownstreamGain() = 0;
    virtual AKRESULT GetParentChannelConfig(AkChannelConfig* out_config) = 0;
    virtual void* GetProcessorFeatures() = 0;
    virtual bool IsPrimary() = 0;
    virtual AKRESULT SignalAudioThread() = 0;
    virtual uint16_t GetNumRefillsInVoice() = 0;
  };

  struct IAkPluginParam {
    // TODO
  };

  struct AkAudioFormat {
    uint32_t uSampleRate;
    AkChannelConfig channelConfig;
    uint32_t uBitsPerSample : 6;
    uint32_t uBlockAlign : 10;
    uint32_t uTypeID : 2;
    uint32_t uInterleaveID : 1;
  };

  struct AkAudioBuffer {
    void* pData;
    AkChannelConfig channelConfig;
    AKRESULT eState;
    uint16_t uMaxFrames;
    uint16_t uValidFrames;

    float* GetChannel(uint32_t index) { return (float*)((char*)this->pData + this->uMaxFrames * sizeof(float) * index); }
  };

  struct AkRamp {
    float fPrev;
    float fNext;
  };

  struct IAkPlugin {
    virtual ~IAkPlugin() = default;
    virtual AKRESULT Term(IAkPluginMemAlloc* allocator) = 0;
    virtual AKRESULT Reset() = 0;
    virtual AKRESULT GetPluginInfo(AkPluginInfo* out_info) = 0;
    virtual bool SupportMediaRelocation() const = 0;
    virtual AKRESULT RelocateMedia(uint8_t* dst, uint8_t* src) = 0;
  };

  struct IAkSinkPlugin : IAkPlugin {
    virtual AKRESULT Init(IAkPluginMemAlloc* in_pAllocator, IAkSinkPluginContext* in_pSinkPluginContext, IAkPluginParam* in_pParams, AkAudioFormat* io_rFormat) = 0;
    virtual AKRESULT IsDataNeeded(uint32_t* out_frames_needed) = 0;
    virtual void Consume(AkAudioBuffer* in_pInputBuffer, AkRamp in_gain) = 0;
    virtual void OnFrameEnd() = 0;
    virtual bool IsStarved() = 0;
    virtual void ResetStarved() = 0;
    virtual void should_not_be_called() { panic("should not be here!"); }
  };

  struct AkPluginRegistration {
    AkPluginRegistration* pNext;
    AkPluginType m_eType;
    uint32_t company_id;
    uint32_t plugin_id;
    IAkPlugin* (*create)(IAkPluginMemAlloc* allocator);
    void* unk[5];
  };
}

// decompiled AK alsa code
namespace ak::raw::alsa {
  struct CAkAlsaSink : IAkSinkPlugin {
    IAkSinkPluginContext* m_pSinkPluginContext{};
    uint32_t m_uOutNumChannels{};
    uint32_t* m_pChannelMap{};
    uint32_t m_uNumBuffers{};
    uint32_t m_uChannelMask{};
    uint16_t m_uFrameSize{};
    int16_t* m_pfSinkOutBuffer{};
    uint32_t m_uUnderflowCount{};
    uint32_t m_uSampleRate{48000};
    snd_pcm_t* m_pPcmHandle{};
    snd_pcm_hw_params_t* m_phwparams{};
    snd_pcm_sw_params_t* m_pswparams{};
    uint32_t m_uOutputID{};
    std::mutex m_threadlock{};
    snd_async_handler_t* m_pPcm_callback_handle{};
    std::optional<std::thread> m_Alsathread{};
    volatile bool m_bDataReady{};
    volatile bool m_bIsPrimary{};
    volatile bool m_bIsRunning{};
    volatile bool m_bCustomThreadEnable{};
    volatile bool m_bThreadRun{};

    CAkAlsaSink() {}

    AKRESULT Term(IAkPluginMemAlloc* allocator) override {
      this->m_bThreadRun = false;
      this->m_bIsRunning = false;
      if (auto thread = std::exchange(this->m_Alsathread, std::nullopt)) {
        thread->join();
      }
      this->m_pPcm_callback_handle = nullptr;
      if (auto params = std::exchange(this->m_pswparams, nullptr)) {
        snd_pcm_sw_params_free(params);
      }
      if (auto params = std::exchange(this->m_phwparams, nullptr)) {
        snd_pcm_hw_params_free(params);
      }
      if (auto handle = std::exchange(this->m_pPcmHandle, nullptr)) {
        snd_pcm_drop(handle);
      }
      if (auto buffer = std::exchange(this->m_pfSinkOutBuffer, nullptr)) {
        allocator->Free(buffer);
      }
      if (auto map = std::exchange(this->m_pChannelMap, nullptr)) {
        allocator->Free(map);
      }
      return AKRESULT::Success;
    }

    AKRESULT Reset() override {
      if (!this->m_pPcmHandle) {
        return AKRESULT::Fail;
      }
      if (this->m_bIsRunning) {
        return AKRESULT::Success;
      }
      if (this->m_bCustomThreadEnable && !this->m_bThreadRun && this->m_bIsPrimary) {
        this->m_bThreadRun = 1;
        this->m_Alsathread.emplace(&AudioThread, this);
        // AlsaThreadProp.nPriority = sched_get_priority_max(1);
      }
      this->m_bIsRunning = 1;
      return AKRESULT::Success;
    }

    AKRESULT GetPluginInfo(AkPluginInfo* out_info) override {
      *out_info = {0x7E30100000007, 0};
      return AKRESULT::Success;
    }

    bool SupportMediaRelocation() const override {
      // not needed
      return false;
    }

    AKRESULT RelocateMedia(uint8_t* dst, uint8_t* src) override {
      // not needed
      return AKRESULT::NotImplemented;
    }

    AKRESULT Init(IAkPluginMemAlloc* in_pAllocator, IAkSinkPluginContext* in_pSinkPluginContext, IAkPluginParam* in_pParams, AkAudioFormat* io_rFormat) override {
      this->m_pSinkPluginContext = in_pSinkPluginContext;
      this->m_bIsPrimary = m_pSinkPluginContext->IsPrimary() & 1;
      uint32_t uDeviceType = 0;
      if (in_pSinkPluginContext->GetOutputID(&this->m_uOutputID, &uDeviceType) != AKRESULT::Success) {
        panic("here1");
        return AKRESULT::NotImplemented;
      }

      const auto global_context = in_pSinkPluginContext->GlobalContext();
      assert_that(global_context != nullptr);
      const auto settings = global_context->GetPlatformInitSettings();

      this->m_uNumBuffers = settings->uNumRefillsInVoice;
      if (this->m_uNumBuffers < 2) {
        panic("buffers: %p %u\n", settings, this->m_uNumBuffers);
        return AKRESULT::Fail;
      }
      this->m_uFrameSize = global_context->GetMaxBufferLength();
      this->m_pChannelMap = (uint32_t*)in_pAllocator->Malloc(64, __FILE__, __LINE__);
      if (!this->m_pChannelMap) {
        return AKRESULT::InsufficientMemory;
      }
      if (snd_pcm_hw_params_malloc(&this->m_phwparams) < 0) {
        return AKRESULT::InsufficientMemory;
      }
      if (snd_pcm_sw_params_malloc(&this->m_pswparams) < 0) {
        return AKRESULT::InsufficientMemory;
      }
      this->m_uOutNumChannels = io_rFormat->channelConfig.num_channels;
      this->m_uChannelMask = io_rFormat->channelConfig.channel_mask;
      if (alsa_stream_connect() != AKRESULT::Success) {
        panic("here1 alsa_stream_connect");
        return AKRESULT::Fail;
      }
      if (!this->m_uOutNumChannels) {
        panic("m_uOutNumChannels: %u\n", this->m_uOutNumChannels);
        return AKRESULT::Fail;
      }
      io_rFormat->channelConfig = AkChannelConfig{
          .num_channels = this->m_uOutNumChannels,
          .config_type = this->m_uChannelMask != 0 ? 1u : 0u,  // standard or anon
          .channel_mask = this->m_uChannelMask,
      };
      io_rFormat->uBitsPerSample = this->m_uOutNumChannels;
      this->m_uSampleRate = settings->uSampleRate;
      io_rFormat->uSampleRate = this->m_uSampleRate;
      for (unsigned i = 0; i < this->m_uOutNumChannels; ++i) {
        unsigned int in_uChannelMask = this->m_uChannelMask;
        unsigned int in_uChannelIdx = i;
        unsigned int uNumChannelsFront = ChannelMaskToNumChannels(in_uChannelMask & 7);
        if ((in_uChannelMask & 8) != 0 && in_uChannelIdx == uNumChannelsFront) {
          in_uChannelIdx = ChannelMaskToNumChannels(in_uChannelMask) - 1;
        } else if (in_uChannelIdx >= uNumChannelsFront) {
          unsigned int uLfeOffset = (in_uChannelMask & 8) != 0;
          if (HasSideAndRearChannels((uint8_t)in_uChannelMask) && in_uChannelIdx < uLfeOffset + uNumChannelsFront + 4) {
            if (in_uChannelIdx < uLfeOffset + uNumChannelsFront + 2)
              in_uChannelIdx += 2;
            else
              in_uChannelIdx -= 2;
          }
          in_uChannelIdx -= uLfeOffset;
        }
        this->m_pChannelMap[i] = in_uChannelIdx;
      }
      this->m_pfSinkOutBuffer = (int16_t*)in_pAllocator->Malloc(this->m_uOutNumChannels * 2LL * this->m_uFrameSize, __FILE__, __LINE__);
      if (!this->m_pfSinkOutBuffer) {
        return AKRESULT::InsufficientMemory;
      }
      memset(this->m_pfSinkOutBuffer, 0, this->m_uOutNumChannels * 2 * this->m_uFrameSize);
      return AKRESULT::Success;
    }

    AKRESULT IsDataNeeded(uint32_t* out_frames_needed) override {
      snd_pcm_sframes_t numberFrameAvailable;  // [rsp+20h] [rbp-20h]

      *out_frames_needed = 0;
      numberFrameAvailable = snd_pcm_avail_update(this->m_pPcmHandle);
      if (numberFrameAvailable < 0) {
        alsa_error_recovery(this, this->m_pPcmHandle, numberFrameAvailable);
        numberFrameAvailable = snd_pcm_avail_update(this->m_pPcmHandle);
        if (numberFrameAvailable < 0) {
          numberFrameAvailable = 0LL;
        }
      }
      *out_frames_needed = numberFrameAvailable / this->m_uFrameSize;
      return AKRESULT::Success;
    }

    void Consume(AkAudioBuffer* in_pInputBuffer, AkRamp in_gain) override {
      float f;
      unsigned int j;
      float fGain;
      int16_t* pfDest;
      float* pfSource;
      unsigned int i;
      float fDelta;
      float fGainStart;
      unsigned int uOutStride;
      unsigned int uNumFrames;

      if (in_pInputBuffer->uValidFrames) {
        uNumFrames = in_pInputBuffer->uValidFrames;
        uOutStride = this->m_uOutNumChannels;
        fGainStart = in_gain.fPrev;
        fDelta = (float)(in_gain.fNext - in_gain.fPrev) / (float)(int)uNumFrames;
        for (i = 0; i < this->m_uOutNumChannels; ++i) {
          pfSource = in_pInputBuffer->GetChannel(this->m_pChannelMap[i]);
          pfDest = &this->m_pfSinkOutBuffer[i];
          fGain = fGainStart;
          for (j = 0; j < uNumFrames; ++j) {
            f = pfSource[j] * fGain;
            if (f < 1.0) {
              if (f > -1.0)
                *pfDest = (int)(float)(f * 32767.0);
              else
                *pfDest = 0x8000;
            } else {
              *pfDest = 0x7FFF;
            }
            pfDest += uOutStride;
            fGain = fDelta + fGain;
          }
        }
        this->m_bDataReady = 1;
      }
    }

    void OnFrameEnd() override {
      int numberByteWritten;
      snd_pcm_sframes_t numberFrameAvailable;

      if (!this->m_bDataReady) {
        memset(this->m_pfSinkOutBuffer, 0, this->m_uOutNumChannels * 2 * this->m_uFrameSize);
      }
      numberFrameAvailable = snd_pcm_avail(this->m_pPcmHandle);
      if (numberFrameAvailable < 0) {
        if (alsa_error_recovery(this, this->m_pPcmHandle, numberFrameAvailable) < 0) {
          this->m_bDataReady = 0;
          return;
        }
        numberFrameAvailable = snd_pcm_avail(this->m_pPcmHandle);
      }
      if (numberFrameAvailable > 0 && numberFrameAvailable >= this->m_uFrameSize) {
        numberByteWritten = snd_pcm_writei(this->m_pPcmHandle, this->m_pfSinkOutBuffer, this->m_uFrameSize);
        if (numberByteWritten < 0) {
          alsa_error_recovery(this, this->m_pPcmHandle, numberByteWritten);
        }
      }
      this->m_bDataReady = 0;
    }

    bool IsStarved() override {
      //
      return this->m_bIsPrimary && this->m_uUnderflowCount != 0;
    }

    void ResetStarved() override {
      //
      this->m_uUnderflowCount = 0;
    }

    static void AudioThread(CAkAlsaSink* self) {
      bool m_bIsPrimary;
      unsigned int nbFrames;
      snd_pcm_sframes_t frameAvail;
      unsigned int uWaitSleepTimeUs;
      signed int uSampleRate;
      uSampleRate = self->m_uSampleRate;
      uWaitSleepTimeUs = (int)(float)((float)((float)((float)self->m_uFrameSize / (float)uSampleRate) * 1000000.0) / 3.0);
      while (self->m_bThreadRun) {
        frameAvail = snd_pcm_avail(self->m_pPcmHandle);
        if (frameAvail < self->m_uFrameSize) {
          usleep(uWaitSleepTimeUs);
        } else {
          nbFrames = frameAvail / self->m_uFrameSize;
          m_bIsPrimary = self->m_bIsPrimary;
          if (m_bIsPrimary) {
            self->m_pSinkPluginContext->SignalAudioThread();
          }
          usleep((int)(float)((float)((float)(int)(self->m_uFrameSize * (unsigned int)(frameAvail / self->m_uFrameSize)) / (float)uSampleRate) * 1000000.0) - uWaitSleepTimeUs);
          // usleep((int)(float)((float)((float)(int)(self->m_uFrameSize * nbFrames) / (float)uSampleRate) * 1000000.0) - uWaitSleepTimeUs);
        }
      }
    }

    static int alsa_error_recovery(CAkAlsaSink* self, snd_pcm_t* in_phandle, int in_err) {
      int result;
      if (in_err == -32) {
        ++(self->m_uUnderflowCount);
        return snd_pcm_prepare(in_phandle);
      } else if (in_err == -86) {
        while (1) {
          result = snd_pcm_resume(in_phandle);
          if (result != -11) {
            break;
          }
          sleep(1u);
        }
        if (result < 0) {
          return snd_pcm_prepare(in_phandle);
        }
        return result;
      } else {
        return in_err;
      }
    }

    AKRESULT alsa_stream_connect() {
      uint32_t v1;                        // eax
      uint32_t v2;                        // eax
      uint32_t v3;                        // eax
      snd_async_handler_t* pcm_callback;  // [rsp+18h] [rbp-78h] BYREF
      unsigned int y;                     // [rsp+24h] [rbp-6Ch]
      int direction;                      // [rsp+28h] [rbp-68h] BYREF
      unsigned int period;                // [rsp+2Ch] [rbp-64h] BYREF
      snd_pcm_uframes_t buffer_size;      // [rsp+30h] [rbp-60h] BYREF
      int i;                              // [rsp+3Ch] [rbp-54h]
      snd_pcm_chmap_query_t* pChMap;      // [rsp+40h] [rbp-50h]
      int iIndexStereo;                   // [rsp+48h] [rbp-48h]
      bool bChannelMapConfigFound;        // [rsp+4Fh] [rbp-41h]
      snd_pcm_chmap_t* FoundChMap;        // [rsp+50h] [rbp-40h]
      snd_pcm_chmap_query_t** ppChMaps;   // [rsp+58h] [rbp-38h]
      unsigned int uSampleRate;           // [rsp+64h] [rbp-2Ch] BYREF
      unsigned int uRate;                 // [rsp+68h] [rbp-28h]
      int retValue;                       // [rsp+6Ch] [rbp-24h]
      snd_pcm_t* pPcmHandle;              // [rsp+70h] [rbp-20h] BYREF

      pPcmHandle = 0LL;
      retValue = snd_pcm_open(&pPcmHandle, "default", SND_PCM_STREAM_PLAYBACK, 1);
      if (retValue < 0 || !pPcmHandle) {
        return AKRESULT::Fail;
      }
      retValue = snd_pcm_hw_params_any(pPcmHandle, this->m_phwparams);
      if (retValue < 0) {
        return AKRESULT::Fail;
      }
      retValue = snd_pcm_hw_params_set_access(pPcmHandle, this->m_phwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
      if (retValue < 0) {
        return AKRESULT::Fail;
      }
      retValue = snd_pcm_hw_params_set_format(pPcmHandle, this->m_phwparams, SND_PCM_FORMAT_S16_LE);
      if (retValue < 0) {
        return AKRESULT::Fail;
      }
      uRate = this->m_uSampleRate;
      uSampleRate = uRate;
      retValue = snd_pcm_hw_params_set_rate_near(pPcmHandle, this->m_phwparams, &uSampleRate, 0LL);
      if (retValue < 0) {
        return AKRESULT::Fail;
      }
      if (uRate != uSampleRate) {
        return AKRESULT::Fail;
      }
      FoundChMap = 0LL;
      ppChMaps = snd_pcm_query_chmaps(pPcmHandle);
      if (ppChMaps) {
        FoundChMap = &(*ppChMaps)->map;
        if (this->m_uOutNumChannels) {
          bChannelMapConfigFound = 0;
          iIndexStereo = -1;
          pChMap = *ppChMaps;
          i = 0;
          while (pChMap) {
            if (pChMap->map.channels == 2) {
              iIndexStereo = i;
            }
            if (pChMap->map.channels == this->m_uOutNumChannels) {
              bChannelMapConfigFound = 1;
              FoundChMap = &ppChMaps[i]->map;
              pChMap = 0LL;
            } else {
              pChMap = ppChMaps[++i];
            }
          }
          if (!bChannelMapConfigFound && iIndexStereo != -1) {
            FoundChMap = &ppChMaps[iIndexStereo]->map;
            this->m_uOutNumChannels = FoundChMap->channels;
            v3 = ChannelMaskFromNumChannels(FoundChMap->channels);
            this->m_uChannelMask = v3;
          }
        } else {
          this->m_uOutNumChannels = FoundChMap->channels;
          v2 = ChannelMaskFromNumChannels(FoundChMap->channels);
          this->m_uChannelMask = v2;
        }
        if (this->m_uOutNumChannels > 0x10) {
          return AKRESULT::Fail;
        }
      } else {
        this->m_uOutNumChannels = 2;
        v1 = ChannelMaskFromNumChannels(this->m_uOutNumChannels);
        this->m_uChannelMask = v1;
      }
      retValue = snd_pcm_hw_params_set_channels(pPcmHandle, this->m_phwparams, this->m_uOutNumChannels);
      if (retValue < 0) {
        return AKRESULT::Fail;
      }
      buffer_size = this->m_uNumBuffers * this->m_uFrameSize;
      retValue = snd_pcm_hw_params_set_buffer_size_near(pPcmHandle, this->m_phwparams, &buffer_size);
      if (retValue < 0) {
        return AKRESULT::Fail;
      }
      period = this->m_uNumBuffers;
      direction = 0;
      retValue = snd_pcm_hw_params_set_periods_near(pPcmHandle, this->m_phwparams, &period, &direction);
      if (retValue < 0) {
        return AKRESULT::Fail;
      }
      retValue = snd_pcm_hw_params(pPcmHandle, this->m_phwparams);
      if (retValue < 0) {
        return AKRESULT::Fail;
      }
      this->m_pPcmHandle = pPcmHandle;
      if (FoundChMap) {
        if (this->m_uOutNumChannels > 1) {
          for (y = 0; y < this->m_uOutNumChannels; ++y) {
            switch (y) {
              case 0u:
                FoundChMap[y + 1].channels = 3;
                break;
              case 1u:
                FoundChMap[y + 1].channels = 4;
                break;
              case 2u:
                FoundChMap[y + 1].channels = 7;
                break;
              case 3u:
                FoundChMap[y + 1].channels = 8;
                break;
              case 4u:
                FoundChMap[y + 1].channels = 5;
                break;
              case 5u:
                FoundChMap[y + 1].channels = 6;
                break;
              case 6u:
                FoundChMap[y + 1].channels = 11;
                break;
              case 7u:
                FoundChMap[y + 1].channels = 9;
                break;
              case 8u:
                FoundChMap[y + 1].channels = 10;
                break;
              case 9u:
                FoundChMap[y + 1].channels = 22;
                break;
              case 0xAu:
                FoundChMap[y + 1].channels = 23;
                break;
              case 0xBu:
                FoundChMap[y + 1].channels = 24;
                break;
              case 0xCu:
                FoundChMap[y + 1].channels = 25;
                break;
              case 0xDu:
                FoundChMap[y + 1].channels = 26;
                break;
              case 0xEu:
                FoundChMap[y + 1].channels = 27;
                break;
              case 0xFu:
                FoundChMap[y + 1].channels = 21;
                break;
              default:
                continue;
            }
          }
          retValue = snd_pcm_set_chmap(pPcmHandle, FoundChMap);
        }
        snd_pcm_free_chmaps(ppChMaps);
      }
      retValue = snd_pcm_sw_params_current(pPcmHandle, this->m_pswparams);
      if (retValue < 0) {
        return AKRESULT::Fail;
      }
      retValue = snd_pcm_sw_params_set_start_threshold(pPcmHandle, this->m_pswparams, 2 * this->m_uFrameSize);
      if (retValue < 0) {
        return AKRESULT::Fail;
      }
      retValue = snd_pcm_sw_params_set_avail_min(pPcmHandle, this->m_pswparams, this->m_uFrameSize);
      if (retValue < 0) {
        return AKRESULT::Fail;
      }
      retValue = snd_pcm_sw_params(pPcmHandle, this->m_pswparams);
      if (retValue < 0) {
        return AKRESULT::Fail;
      }
      if (this->m_bIsPrimary) {
        retValue = snd_async_add_pcm_handler(&pcm_callback, pPcmHandle, (snd_async_callback_t)audio_cb, this);
        if (retValue >= 0)
          this->m_pPcm_callback_handle = pcm_callback;
        else
          this->m_bCustomThreadEnable = 1;
      }
      retValue = snd_pcm_prepare(pPcmHandle);
      if (retValue >= 0)
        return AKRESULT::Success;
      else
        return AKRESULT::Fail;
    }

    static void audio_cb(snd_async_handler_t* pPcmCallback) {
      if (!pPcmCallback) {
        return;
      }
      auto pAlsaSinkDevice = (CAkAlsaSink*)snd_async_handler_get_callback_private(pPcmCallback);
      std::lock_guard lock(pAlsaSinkDevice->m_threadlock);
      snd_pcm_t* pPcmHandle = snd_async_handler_get_pcm(pPcmCallback);
      snd_pcm_sframes_t frameAvail = snd_pcm_avail_update(pPcmHandle);
      if (frameAvail < 0) {
        if (alsa_error_recovery(pAlsaSinkDevice, pPcmHandle, frameAvail) >= 0) {
          frameAvail = snd_pcm_avail_update(pPcmHandle);
          if (frameAvail < 0) {
            frameAvail = 0LL;
          }
        } else {
          frameAvail = 0LL;
        }
      }
      if (frameAvail >= pAlsaSinkDevice->m_uFrameSize && pAlsaSinkDevice->m_bIsPrimary) {
        pAlsaSinkDevice->m_pSinkPluginContext->SignalAudioThread();
      }
    }

    static bool HasSideAndRearChannels(uint8_t in_uChannelMask) { return (in_uChannelMask & 0x210) != 0; }

    static uint8_t ChannelMaskToNumChannels(uint8_t in_uChannelMask) {
      uint8_t num = 0;
      while (in_uChannelMask) {
        ++num;
        in_uChannelMask &= in_uChannelMask - 1;
      }
      return num;
    }

    static uint32_t ChannelMaskFromNumChannels(unsigned int in_uNumChannels) {
      switch (in_uNumChannels) {
        case 1u:
          return 4;
        case 2u:
          return 3;
        case 3u:
          return 11;
        case 4u:
          return 1539;
        case 5u:
          return 1543;
        case 6u:
          return 1551;
        case 7u:
          return 1591;
        case 8u:
          return 1599;
        default:
          return 0;
      }
    }

    static IAkPlugin* create(IAkPluginMemAlloc* allocator) {
      // create:
      return new (allocator->Malloc(sizeof(CAkAlsaSink), __FILE__, __LINE__)) CAkAlsaSink();
    }
  };
};

void ak::patch_sink(loader::Image* image) {
  // entries are laid out as singly linked list entries
  struct CAkPluginNode {
    CAkPluginNode* next;
  };
  CAkPluginNode** g_pAKPluginList = nullptr;
  CAkPluginNode* DefaultSinkRegistration = nullptr;
  auto const base = image->object.text_segment()->vmaddr + image->slide;
  for (auto const e : image->object.exports) {
    if (e.name == "_g_pAKPluginList") {
      g_pAKPluginList = (CAkPluginNode**)(base + e.symbol_offset.value_or(0));
    }
    if (e.name == "_DefaultSinkRegistration") {
      DefaultSinkRegistration = (CAkPluginNode*)(base + e.symbol_offset.value_or(0));
    }
  }

  if (!g_pAKPluginList || !DefaultSinkRegistration) {
    return;
  }

  for (auto i = *g_pAKPluginList; i; i = i->next) {
    if (i == DefaultSinkRegistration) {
      auto r = (ak::raw::AkPluginRegistration*)i;
      r->create = &ak::raw::alsa::CAkAlsaSink::create;
    }
  }

  // CAkPluginNode** i = g_pAKPluginList;
  // while (i && *i) {
  //   if (*i == DefaultSinkRegistration) {
  //     // remove entry from singly linked list
  //     *i = (*i)->next;
  //   }
  //   i = &(*i)->next;
  // }
}
