# Tahoma
Programm der Serie ProWo zum Auslesen der Somfy Tahoma-Box. 
Das Programm läuft auf einem Raspberry Pi.

Programm kompilieren mit make -f Makefile

Starten mit “./Tahoma”, die Daten werden aus der Datei Tahoma.config gelesen: 
    1° Wenn in der Datei SOMFYPIN und SOMFYTOKEN definiert sind werden alle Daten der Somfygeräte eingelesen.
    
    2° ist SOMFYTOKEN nicht definiert, müssen neben dem SOMFYPIN auch SOMFYUSER und SOMFYPWD (gründen mit Tahoma-App) definiert sein.
       Es werden dann auch alle Somfygeräte eingelesen.

Das Resultat befindet sich in der Datei prowo.config. 
In der Datei Tahoma.json befindet sich die komplette Antwort der Tahoma-Box.
