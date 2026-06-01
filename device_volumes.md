# TgDeviceVolume — Hazard Devices

Common denominator: `asm_data_set_devices.slot_used_value_id = 1354` → `asm_data_set_valid_values.text_msg_translated = "Volumes"`.

16 devices total.

| device_id | mode_id | name (msg) | target          | effect_group_id(s) | refire (s) | accuracy | range |
|----------:|--------:|------------|-----------------|--------------------|-----------:|---------:|------:|
| 2238      | 2750    | KillEffect    | Enemy           | 7010               | 0.1        | 1.0      | —     |
| 2801      | 3223    | Invulnerable  | Friend and Self | 8698               | 1.0        | 1.0      | —     |
| 2805      | 3225    | Invulnerable  | Friend Only     | 8882               | 0.2        | 1.0      | 50    |
| 4662      | 4231    | Magma Burn    | All             | 16864, 16865       | 0.25       | —        | —     |
| 4976      | 4406    | Poison        | All             | 17711, 17712       | 1.0        | —        | —     |
| 5079      | 4427    | Shock         | Enemy           | 18904, 18905       | 0.5        | —        | —     |
| 5133      | 4445    | Magma Burn    | All             | 19070, 19071       | 0.25       | —        | —     |
| 5299      | 4454    | Poison        | All             | 20280, 20281       | 0.5        | —        | —     |
| 5447      | 4457    | Magma Burn    | All             | 21502, 21503       | 0.25       | —        | —     |
| 5448      | 4458    | KillEffect    | Enemy           | 21504              | 0.1        | 1.0      | —     |
| 5449      | 4459    | KillEffect    | Enemy           | 21505              | 0.1        | 1.0      | —     |
| 5450      | 4460    | KillEffect    | Enemy           | 21506              | 0.1        | 1.0      | —     |
| 6265      | 4600    | Magma Burn    | Enemy           | 25337              | 1.0        | —        | —     |
| 7029      | 4728    | Magma Burn    | All             | 26167, 26168       | 0.25       | —        | —     |
| 7037      | 4737    | Poison        | All             | 26194, 26195       | 0.5        | —        | —     |
| 7178      | 4756    | KillEffect    | Enemy           | 26502              | 0.1        | 1.0      | —     |

All entries: `in_hand_device_flag = 0`, `container_skill_group_id = 0`, `damage_class_res_id = 0`, `device_projectile_id = 0`, effect groups are `Hit` (`effect_group_type_value_id = 264`). `class_res_id = 4 (TgGame.TgDeployable)` on all except device 4976 (`class_res_id = 0`).

Property ids: 5 = Range, 10 = Accuracy, 53 = Refire Time.
