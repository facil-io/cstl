## Hash Function Testing

During development I tested the Hash functions using [the SMHasher testing suite (@rurban's fork)](https://github.com/rurban/smhasher). The testing suite is often growing with more tests, so I do not know what the future may bring... but attached are the test results for Risky Hash, Stable Hash (64 and 128 bit variation) as they were at the time of (re)development (March, 2023).

**Results status (October 2026)**:

- **Risky Hash, Stable Hash (64 and 128 bit)**: the current code produces the same SMHasher verification values as the results below (`0x407D2C05`, `0x6993BB92`, `0x97D1AB52`), so the quality results still apply to the current output. A re-run against the current SMHasher test set is pending; speed figures may be out of date.
- **Risky256 / Risky512**: added later; not yet tested.
