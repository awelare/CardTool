@echo off
pio run -e cardputer
if errorlevel 1 (echo ОШИБКА СБОРКИ & exit /b 1)
echo.
echo Готово: %~dp0.pio\build\cardputer\firmware.bin
echo Копируй на SD через "Копировать", не двойным кликом.
