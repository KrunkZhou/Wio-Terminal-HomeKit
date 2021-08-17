# homebridge-kiot

config.json

```
{
    "bridge": {
        "name": "HomeBridge",
        "username": "0A:81:3D:E3:84:2B",
        "port": 53780,
        "pin": "031-45-868"
    },
    "platforms": [
        {
            "name": "Config",
            "port": 8581,
            "auth": "form",
            "theme": "auto",
            "tempUnits": "c",
            "lang": "zh-CN",
            "platform": "config"
        }
    ],
    "disabledPlugins": [],
    "accessories": [
        {
            "accessory": "KIOTSwitch",
            "name": "LivingRoom Switch KIOT",
            "port": 44002
        },
        {
            "accessory": "KIOTSwitch",
            "name": "SyncBox Switch KIOT",
            "port": 44003
        },
        {
            "accessory": "KIOTSwitch",
            "name": "ServerFan Switch KIOT",
            "port": 44004
        },
        {
            "accessory": "KIOTSwitch",
            "name": "DinningRoom Switch KIOT",
            "port": 44005
        },
        {
            "accessory": "KIOTSwitch",
            "name": "Nanoleaf Switch KIOT",
            "port": 44006
        },
        {
            "accessory": "KIOTSwitch",
            "name": "GarageDoor Switch KIOT",
            "port": 44007
        }
    ]
}
```

Press Once
```
http://<HB-ip>:port/kiot-button/1
```
Press Twice
```
http://<HB-ip>:port/kiot-button/2
```
Long Press
```
http://<HB-ip>:port/kiot-button/3
```
