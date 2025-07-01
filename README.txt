# Projet Yharnam – Injecteur PE64 statique

Ce dépôt implémente l'injecteur demandé par le sujet « Virologie & Malware ».

## Architecture globale

* **injector.cpp** – Injecteur statique (Windows x64)  
  1. Ouvre l'exécutable cible.  
  2. Crée une nouvelle section « .yhar » exécutable/lecture/écriture.  
  3. Patch l'`AddressOfEntryPoint` du PE vers cette nouvelle section.  
  4. Copie la payload position‑indépendante (`payload.bin`).  
  5. Insère l'OEP d'origine à la fin de la payload pour retour propre.  

* **payload.asm** – Stub position‑indépendant (x86‑64)  
  – Affiche `MessageBoxA("Infection reussie!")`.  
  – Rend la main à l’OEP original patché par l’injecteur.  

* **Makefile** – Build Linux _(cross‑compile mingw‑w64)_.  
  ```bash
  sudo apt install gcc-mingw-w64 nasm
  make                # → injector.exe & payload.bin
  ```

## Compilation native Windows (Visual Studio)

1. Assemblez la payload :
   ```bat
   nasm -f bin payload.asm -o payload.bin
   ```
2. Ouvrez un Developer Prompt x64 puis :
   ```bat
   cl /O2 /MT /nologo injector.cpp /link /out:injector.exe
   ```

## Utilisation

```console
injector.exe original.exe
```
* Sortie : `original_infected.exe`  
* Au premier lancement, une MessageBox confirme l'infection, puis le
  programme légitime s’exécute normalement.

## Bonus déjà pris en charge
* **Injection dynamique de répertoire** : la cible peut être passée comme
  argument ; il est trivial de boucler sur `*.exe` si souhaité.
* **Infection de process** : le code est prêt à être étendu (voir TODO).
* **Section packable** : la section `.yhar` est alignée correctement pour
  une compression UPX (`upx --best original_infected.exe`).

## Limitations & Pistes d'amélioration

* Le stub résout `MessageBoxA` via l'IAT ; si la cible n'importe pas
  `user32.dll`, ajouter un resolveur dans la payload.
* Pas de support des fichiers PE TLS ou protégés par signature authenticode.
* Non‑prise en charge des sections alignées < 4 Kio.

## Rendu

1. Mettez à jour `AUTHORS.txt`.  
2. Lancez `make clean && make`.  
3. Archivez tout le répertoire :
   ```bash
   7z a -t7z -p'yharnam' -mhe=on yharnam.7z *
   ```
4. Uploadez `yharnam.7z` sur Google Classroom.
