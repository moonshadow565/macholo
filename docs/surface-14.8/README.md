# Import surface — League 14.8 (thin x86_64)

Regression-baseline target. Only `libSystem.txt` is captured here (generated with
`build/macholo-dump --syms` like the other surface dirs) — it exists so the Track 3
audit's demand universe covers 14.8 too; 13 of its libSystem imports don't appear
in 16.3/16.15 (fortify/wchar/math stragglers dropped by newer SDKs).
