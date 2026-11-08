# Veltrane Sampler (VST3)

Сэмплер-плагин для FL Studio и любых других DAW с поддержкой VST3.
Загружает папку Veltrane Sound Pack (киков, снейров, хэтов и синтов по 500 звуков),
позволяет листать и искать звуки, слушать их и играть с клавиатуры/MIDI.

## Возможности
- Браузер библиотеки: категории (папки), поиск, мгновенное прослушивание
- 16 голосов, интерполяция Hermite (чистое питч-шифтирование)
- Корневая нота берётся из имени файла (Kick_808_339_A1 -> A1, Synth_Bell_412_As4 -> A#4)
- Ручки: Gain, Attack, Release, Cutoff (low-pass), Pitch (+-24 полутона)
- Режим One-shot: любая нота играет звук в оригинальной высоте и не обрывается (для ударных;
  включается автоматически в категориях Snares и HiHats)
- Выбранный звук и папка библиотеки сохраняются вместе с проектом FL Studio

## Как получить готовый .vst3 БЕЗ установки компиляторов (GitHub Actions)
1. Создайте бесплатный аккаунт на github.com и новый репозиторий (Private подойдёт).
2. Загрузите в него все файлы этого проекта (в корне должны лежать CMakeLists.txt, Source/, .github/).
3. Вкладка Actions -> дождитесь зелёной галочки у "Build VST3" (5-15 минут).
4. Откройте завершённый запуск -> внизу Artifacts -> скачайте VeltraneSampler-VST3-Windows.
5. Распакуйте и скопируйте папку "Veltrane Sampler.vst3" в
   C:\Program Files\Common Files\VST3\

## Как собрать самому (Windows)
Нужны: Visual Studio 2022 (Desktop development with C++), CMake, Git.
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
Результат: build\VeltraneSampler_artefacts\Release\VST3\Veltrane Sampler.vst3
(первая сборка скачает JUCE 8.0.4 из GitHub, нужен интернет)

## Подключение в FL Studio 21
1. Options -> Manage plugins -> Find installed plugins (Start scan).
2. Добавьте "Veltrane Sampler" из Plugin database -> Installed -> Generators.
3. В плагине нажмите "Library folder..." и выберите распакованную папку Veltrane Sound Pack
   (внутри должны быть Kicks, Snares, HiHats, Synth).
4. Выберите категорию, кликните звук в списке - он загрузится и прозвучит.

## Лицензия
Плагин собран на фреймворке JUCE (AGPLv3 / коммерческая лицензия). Для личного использования
это бесплатно. Если захотите продавать или распространять собранный плагин, нужно выполнить
условия AGPL (открыть исходники) либо купить лицензию JUCE.
