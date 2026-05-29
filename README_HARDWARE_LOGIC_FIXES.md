# Hardware Logic Fixes

## YOLO UNO Board Mapping

```text
D13 = GPIO48
D12 = GPIO47
D11 = GPIO38
D7  = GPIO10
SDA = GPIO11
SCL = GPIO12
```

## Final Hardware Wiring

```text
DHT20 SDA            -> GPIO11 / SDA
DHT20 SCL            -> GPIO12 / SCL

PIR OUT/SIG          -> GPIO38 / D11
Light sensor AO/SIG  -> GPIO10 / D7

Fan relay IN         -> GPIO47 / D12
Auto-light LED D13   -> GPIO48 / D13
```

## Main Problem Fixed

Before the fix, GPIO48 was used for the fan relay. On the YOLO UNO board, GPIO48 is D13, so D13 turned ON whenever the fan was activated.

After the fix:

- Fan relay uses GPIO47 / D12.
- D13 / GPIO48 is used for the auto-light LED condition.

## GPIO12 Conflict

The project uses I2C with:

```cpp
Wire.begin(11, 12);
```

That means:

- GPIO11 is SDA
- GPIO12 is SCL

Therefore, GPIO12 must not be used as a relay output or LED output.

## Final Pin Roles

```cpp
#define LIGHT_PIN             10    // D7, analog input from light sensor AO/SIG
#define PIR_PIN               38    // D11, digital input from PIR OUT/SIG

#define RELAY_FAN_PIN         47    // D12, fan relay output
#define RELAY_LIGHT_PIN       48    // D13, auto-light LED output

#define FAN_TEMP_THRESHOLD    27.0f
#define LIGHT_THRESHOLD       1500
```

## Final Auto-Control Behavior

### Fan Logic

Fan turns ON when:

```cpp
human_inside == true && temperature > FAN_TEMP_THRESHOLD
```

Meaning:

```text
PIR detects motion AND temperature > 27°C -> Fan ON
Otherwise -> Fan OFF
```

### D13 Auto-Light LED Logic

D13 auto-light LED turns ON when:

```cpp
human_inside == true && light_level < LIGHT_THRESHOLD
```

Meaning:

```text
PIR detects motion AND room is dark -> D13 ON
Otherwise -> D13 OFF
```

### When Both Should Be ON

If all three conditions are true:

```text
PIR detects motion
Room is dark
Temperature > 27°C
```

then both the fan and D13 should be ON at the same time.

## Sensor Roles

- PIR sensor detects motion only.
- PIR cannot detect brightness.
- Light sensor detects whether the room is bright or dark.
- DHT20 detects temperature and humidity.

## Testing Table

| Case | PIR Motion | Room Dark | Temp > 27°C | Expected D13 | Expected Fan |
| ---- | ---------- | --------- | ----------- | ------------ | ------------ |
| 1    | No         | Any       | Any         | OFF          | OFF          |
| 2    | Yes        | No        | No          | OFF          | OFF          |
| 3    | Yes        | Yes       | No          | ON           | OFF          |
| 4    | Yes        | No        | Yes         | OFF          | ON           |
| 5    | Yes        | Yes       | Yes         | ON           | ON           |

## Troubleshooting

- If D13 still follows fan, check that fan relay IN is physically connected to D12/GPIO47, not D13/GPIO48.
- If D13 never turns ON, check light sensor AO/SIG is connected to GPIO10/D7.
- If PIR always detects motion, check PIR wiring, warm-up time, sensitivity, and GND.
- If fan does not turn ON, check temperature is really above 27°C and PIR is detecting motion.
- If upload fails on Windows, use a COM port instead of `/dev/ttyACM0`.

## Build Commands

Build:

```bash
pio run -e yolo_uno
```

Upload:

```bash
pio run -e yolo_uno -t upload
```

## Final Summary

```text
GPIO10 / D7   -> Light sensor AO/SIG
GPIO38 / D11  -> PIR OUT/SIG
GPIO47 / D12  -> Fan relay
GPIO48 / D13  -> Auto-light LED
GPIO11 / SDA  -> DHT20 SDA
GPIO12 / SCL  -> DHT20 SCL
```
