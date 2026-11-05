#!/usr/bin/env python3
"""Create a standalone StopWatch image and verify its component bytes. No flashing."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
BUILD=ROOT/'build'
OUT=ROOT/'release'
OUT.mkdir(exist_ok=True)
description=json.loads((BUILD/'project_description.json').read_text())
assert description['target']=='esp32s3'
assert Path(description['project_path']).resolve()==ROOT
config=(ROOT/'sdkconfig').read_text()
assert 'CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y' in config
assert 'CONFIG_SPIRAM_MODE_OCT=y' in config
flash=json.loads((BUILD/'flasher_args.json').read_text())
expected={'0x0':'bootloader/bootloader.bin','0x8000':'partition_table/partition-table.bin','0x10000':'grok_bot_stopwatch.bin'}
assert flash['flash_files']==expected,flash['flash_files']
parts=[]
for offset,relative in expected.items():
    source=BUILD/relative
    name=source.name
    shutil.copy2(source,OUT/name)
    parts.append({'offset':offset,'file':name,'size':source.stat().st_size,
                  'sha256':hashlib.sha256(source.read_bytes()).hexdigest()})
merged=OUT/'grok-bot-stopwatch-merged.bin'
settings=flash['flash_settings']
assert settings['flash_size']=='16MB'
command=[sys.executable,'-m','esptool','--chip','esp32s3','merge_bin','-o',str(merged),
         '--flash_mode',settings['flash_mode'],'--flash_freq',settings['flash_freq'],
         '--flash_size',settings['flash_size']]
for offset,relative in expected.items(): command += [offset,str(BUILD/relative)]
subprocess.run(command,check=True)
image=merged.read_bytes()
for part in parts:
    offset=int(part['offset'],16)
    # Use the IDF-generated flash settings so the bootloader header stays unchanged.
    assert image[offset:offset+part['size']]==(OUT/part['file']).read_bytes()
sources={}
for folder in ('main','components','scripts','tests'):
    for path in sorted((ROOT/folder).rglob('*')):
        if path.is_file() and '__pycache__' not in path.parts:
            sources[str(path.relative_to(ROOT))]=hashlib.sha256(path.read_bytes()).hexdigest()
for name in ('CMakeLists.txt','sdkconfig.defaults','partitions.csv','dependencies.lock'):
    sources[name]=hashlib.sha256((ROOT/name).read_bytes()).hexdigest()
(OUT/'source-sha256.json').write_text(json.dumps(sources,indent=2)+'\n')
shutil.copy2(ROOT/'dependencies.lock',OUT/'dependencies.lock')
shutil.copy2(ROOT/'sdkconfig',OUT/'sdkconfig')
notices=OUT/'licenses';notices.mkdir(exist_ok=True)
sdk_license=Path(description['idf_path'])/'LICENSE'
shutil.copy2(sdk_license,notices/'ESP-IDF-LICENSE')
for component in (ROOT/'managed_components').iterdir():
    for path in component.rglob('*'):
        if path.is_file() and path.name.lower().startswith(('license','copying','ofl')):
            name=str(path.relative_to(ROOT/'managed_components')).replace('/','__')
            shutil.copy2(path,notices/name)
manifest={'project':'grok-bot-esp32','version':description['project_version'],'board':'M5Stack StopWatch C152',
          'chip':'esp32s3','flash_size':16777216,'idf_version':description['git_revision'],
          'merged_image':{'file':merged.name,'offset':'0x0','size':len(image),
                          'sha256':hashlib.sha256(image).hexdigest()},
          'parts':parts,'flashed':False,'hardware_verified':False,
          'controls':{'left':'next shape (320 ms single-click delay)','left_double':'calibrate neutral IMU pose; hold still briefly','right':'next color','touch':'pointer position',
                      'tilt':'head orientation when not touching','both_hold_1s':'recenter IMU'},
          'credits':[{'name':'Grok Bot web reference','url':'https://grokbot.yin.wiki/index.html'},{'name':'xAI Grok Bot visual inspiration','url':'https://x.ai/bot'}],
          'note':'Merged image replaces the bootloader, partition table and application; intended for this standalone demo.'}
(OUT/'manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
print(json.dumps(manifest['merged_image'],indent=2))
