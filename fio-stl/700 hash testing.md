## Hash Function Testing

During development I tested the Hash functions using [the SMHasher testing suite (@rurban's fork)](https://github.com/rurban/smhasher). The testing suite is often growing with more tests, so I do not know what the future may bring... but attached are the test results for Risky Hash, Stable Hash (64 and 128 bit variation) and Risky256 / Risky512. The summary below lists when, where and against which version each result was produced.

### SMHasher Summary

Generated from the latest full runs (SMHasher e154a82; Apple M4 Max). The full logs follow. SHA-2 rows are reference baselines run with the same tests and seeding.

| Hash | Verification | Failures (`!!!!`) | Warnings (`!`) | Worst avalanche bias | Bulk MiB/s | Small keys (1–32 B) ns/hash | Run |
|---|---|---|---|---|---|---|---|
| Risky Hash | `0x407D2C05` PASS | 0 | 0 | 0.835% | 34,416 | 8.0 | 2026-11-01, `615c171` |
| Stable Hash 64 | `0x6993BB92` PASS | 0 | 0 | 0.825% | 23,749 | 12.5 | 2026-11-01, `615c171` |
| Stable Hash 128 | `0x97D1AB52` PASS | 0 | 0 | 0.773% | 23,619 | 12.6 | 2026-11-01, `615c171` |
| Risky256 (keyed, key = seed) | `0xE6AD4AC9` PASS | 0 | 0 | 0.854% | 16,070 | 31.4 | 2026-11-01, `615c171` |
| Risky256, low 64 bits | `0x71DF5C7A` PASS | 0 | 1 | 0.843% | 16,106 | 31.5 | 2026-11-01, `615c171` |
| Risky512, high 256 bits | `0x52F03B1D` PASS | 0 | 0 | 0.828% | 16,285 | 30.8 | 2026-11-01, `615c171` |
| SHA-512, high 256 bits | `0x978878D8` PASS | 0 | 1 | 0.825% | 880 | 150.0 | 2026-11-01, `615c171` |
| SHA-512, low 256 bits | `0x77AA6DD3` PASS | 0 | 0 | 0.849% | 887 | 147.4 | 2026-11-01, `615c171` |
| SHA-256 | `0xEBDA2FB1` PASS | 0 | 0 | 0.814% | 456 | 149.1 | 2026-11-01, `615c171` |
| SHA-256, low 64 bits | `0xC1C4FA72` PASS | 0 | 0 | 0.862% | 407 | 161.5 | 2026-11-01, `615c171` |

Warnings (`!`) mark single results outside the expected range (e.g., 1 collision where 0.3 are expected); a few appear by chance in any long run, SHA-2 included. Failures (`!!!!`) indicate real defects. Speed figures depend on the machine and compiler.
