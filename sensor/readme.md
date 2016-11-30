# Temperature & Humidity Sensor Evaluation and Testing



| Metric              | [HTU21D](https://octopart.com/htu21d-measurement+specialties-30374934)             |[Si72021](https://octopart.com/si7021-a20-im-silicon+labs-52675246)| DHT22          | DHT22 vs HTU21D Difference| [BME280](https://octopart.com/bme280-bosch-52931981) |
| ------------- | --------|------------- | ------------- | ------------- | ------------- |
| Cost in 1k off      | £2.15|  £4.57 (July16)    | 3.2 times cheaper (£3.15 less!) | £3.07 |
| Vcc                 |  1.9V - 3.6V | 3.3-6V | | 1.71 V - 3.6V |
| Humidity accuracy   |  ±2% RH |  ±2%RH         | n/a | ±3%RH |
| Humidity Range      |  0-100% RH |  0-100% RH     | n/a|  0-100% RH  |
| Temperature accuracy | ±0.4°C|  ±0.5°C        | 40% more accurate | ±0.5 - ±1.0 |
| Temperature Range   |   -40°C +125°C  |  -40°C +80°C   | 56% more accurate | -40°C +80°C  |
| Sleep Current       | 0| 0.06uA |  15uA          | 750 times less power | 0.1uA - 0.3uA |
| Measurement Current | 0. 0.09 mA |  0.5mA         | 11 times less power | 0.714mA - 0.350mA |
| Measurement time    | 0.01s | 0.01s - 0.0026s |  2s            | 200 times faster | 0.013s |
| Energy consumed per sample| 2.97uJ |  3300uJ           | 2000 times less power | |
| Time sampling per day* ||  2800s         | | |
| Time sleeping per day* | |  83600s        | | |
| Energy consumed per day*  ||  8700mJ [2]    | 1115 times less energy per day!  | |

```
energy consumed (joule, J) = potential difference (volt, V) × charge (coulomb, C)
charge (coulomb, C) = current (ampere, A) × time (second, s)
energy consumed (joule, J)  = potential difference (volt, V) × current (ampere, A) × time (second, s)
```

## Energy consumed per day*

Energy consumed per day = energy consumed per sample + energy consumed while sleeping.

- **HTU21D:** (1400 * 0.0015mJ) + (3.3V * 0.00002mA * 86386 ) = 2.1mJ + 5.7mJ = 7.8mJ [1]
- **DHT22:**  (1400 * 3.3mJ) + (3.3V * 0.015mA * 83600) = 4.6mJ + 4.1J = 8.7J = 8700mJ   [2]

\*Assuming 1 sample per min and sleeping in between samples: 1400 samples per day

[Adafruit HTU21D Library](https://github.com/adafruit/Adafruit_HTU21DF_Library)

