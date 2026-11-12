# VLoader

Загрузчик Android-игр для PS Vita. Берёт нативную библиотеку игры (`.so` под armeabi-v7a) из APK, загружает её как есть и подставляет вместо Android свои функции: система, файлы, OpenGL ES через vitaGL, поддельный JNI.

Отличие от обычных so-loader портов: игра описывается текстовым профилем, а не правкой кода. Один и тот же загрузчик запускает разные игры, у каждой свой пузырь и своя папка `ux0:data/vloader/TITLEID/`.

Проект в ранней стадии. На консоли проверена только тестовая игра из `testgame/`. Профиль покрывает игры, которые общаются с Java через функции `Java_*` (init, кадр, тач, кнопки). Игры на NativeActivity, Unity и Unreal пока не поддерживаются.

## Что нужно на Вите

- плагин `kubridge.skprx` в `*KERNEL`;
- `libshacccg.suprx` в `ur0:data/`;
- `VLoader.vpk`;
- папка игры `ux0:data/vloader/TITLEID/` с библиотекой игры, `profile.txt` и ресурсами.

Лог запуска и сообщения игры пишутся в `ux0:data/vloader/TITLEID/log.txt`.

## Профиль

```
name=Test Game
so=libgame.so
onload=1

init=Java_com_vpkgj_test_Native_init env class width height
frame=Java_com_vpkgj_test_Native_step env class
touch=Java_com_vpkgj_test_Native_touch env class action x y
key=Java_com_vpkgj_test_Native_key env class keycode down

touch.down=1
touch.up=0
touch.move=2

java.int.getRefreshRate=60
java.string.getLanguage=ru
java.string.getDataPath=@data
```

| Ключ | Смысл |
|---|---|
| `so` | библиотека игры в папке игры |
| `onload` | вызывать ли `JNI_OnLoad` |
| `start` (или `init`) | функция игры и её аргументы; строк может быть несколько, вызываются по порядку при запуске |
| `frame`, `touch`, `key`, `analog`, `pause`, `resume` | функция игры и её аргументы по порядку |
| `mkdir` | папка, которую создать перед запуском |
| `keep_apk` | `1`, если игре нужен сам APK: VPKGJ оставит его как `game.apk` в папке игры |
| `touch.down`, `touch.up`, `touch.move` | какие числа игра ждёт в `action` |
| `width`, `height` | размер экрана, который увидит игра (по умолчанию 960x544) |
| `java.int.ИМЯ`, `java.bool.`, `java.float.`, `java.long.`, `java.string.`, `java.void.` | ответ на вызов Java-метода с таким именем |

Аргументы функций: `env`, `vm`, `class`, `obj`, `width`, `height`, `x`, `y` (дробные), `xn`, `yn` (от 0 до 1), `xi`, `yi` (целые), `id`, `action`, `keycode`, `down`, `stick`, `time` (миллисекунды), `pad` (пустое слово для выравнивания 64-битных значений), `str:текст` (Java-строка), а также числа (`1`, `0x10`, `1.5f`). Всего до 12 слов. Значение `@data` заменяется на путь к папке игры. Строки с `#` считаются комментариями.

## Проверка APK

```
python3 tools/apkcheck.py game.apk
python3 tools/apkcheck.py game.apk --out vloader/TITLEID --profile profile.txt
```

Показывает архитектуры, движок, точки входа, функции `Java_*` и системные функции, которых нет в загрузчике. С `--out` раскладывает библиотеки и assets в папку игры.

## Сборка

Нужен softfp-вариант VitaSDK (https://github.com/vitasdk-softfp) с пакетами `zlib taihen kubridge libmathneon vitaShaRK SceShaccCgExt libogg libvorbis flac opus mpg123 lame libsndfile opensles`.

```
export VITASDK=/путь/к/vitasdk-softfp
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Тестовая игра собирается через Android NDK и SDK:

```
sh testgame/build.sh
sh testgame/build_apk.sh
```

## На чём основано

- [soloader-boilerplate](https://github.com/v-atamanenko/soloader-boilerplate) (Volodymyr Atamanenko), MIT
- [so_util](https://github.com/Rinnegatamante/so_util) (TheFloW, Rinnegatamante)
- [FalsoJNI](https://github.com/v-atamanenko/FalsoJNI), [FalsoNDK](https://github.com/elliencode/FalsoNDK)
- [vitaGL](https://github.com/Rinnegatamante/vitaGL) (Rinnegatamante), LGPL-3.0
- [kubridge](https://github.com/bythos14/kubridge)

Лицензии этих проектов лежат рядом с их исходниками в `lib/`. Собственный код VLoader распространяется по MIT, см. `LICENSE`.
