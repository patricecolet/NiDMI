#!/usr/bin/env python3
"""Moniteur serie robuste pour NiDMI / ESP32 (adapte a l'USB CDC natif S3).

Pourquoi pas `arduino-cli monitor` : sur l'USB CDC natif, le port disparait a
chaque reboot/crash/re-enumeration et arduino-cli s'arrete. Ce moniteur :

- auto-detecte le port CDC de l'ESP si --port absent ;
- ne pulse JAMAIS RTS/DTR (evite le passage accidentel en mode download/bootloader
  sur ESP32-S3) ;
- se reconnecte automatiquement quand le port tombe -> capture les boucles de
  reboot et les backtraces de panic au lieu de mourir ;
- horodate chaque ligne et duplique la sortie dans un fichier sous .logs/.

Usage :
  python3 scripts/serial_monitor.py                 # auto-detection du port
  python3 scripts/serial_monitor.py --port /dev/cu.usbmodemXXXX
  python3 scripts/serial_monitor.py --baud 115200
"""
import sys
import os
import time
import glob
import argparse
import datetime

try:
    import serial  # pyserial (livre avec esptool)
except ImportError:
    sys.exit("pyserial manquant : python3 -m pip install pyserial")


def detect_port():
    cands = (glob.glob('/dev/cu.usbmodem*') + glob.glob('/dev/cu.wchusbserial*')
             + glob.glob('/dev/ttyACM*') + glob.glob('/dev/ttyUSB*'))
    # Le port CDC a un nom court (ex. usbmodem11301) ; le port JTAG hardware du S3
    # contient la MAC (nom long). On prefere donc le plus court.
    cands.sort(key=len)
    return cands[0] if cands else None


def open_port(port, baud):
    s = serial.Serial()
    s.port = port
    s.baudrate = baud
    s.timeout = 0.2
    # Ne pas asserter les lignes de reset/boot.
    s.dtr = False
    s.rts = False
    s.open()
    try:
        s.dtr = False
        s.rts = False
    except Exception:
        pass
    return s


def ts():
    return datetime.datetime.now().strftime('%H:%M:%S.%f')[:-3]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--port')
    ap.add_argument('--baud', type=int, default=115200)
    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    ap.add_argument('--logdir', default=os.path.join(repo_root, '.logs'))
    args = ap.parse_args()

    os.makedirs(args.logdir, exist_ok=True)
    logpath = os.path.join(args.logdir, 'serial-%s.log'
                           % datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
    logf = open(logpath, 'a', buffering=1)

    def emit(text):
        sys.stdout.write(text)
        sys.stdout.flush()
        logf.write(text)

    emit('[monitor] log -> %s\n' % logpath)
    emit('[monitor] Ctrl-C pour quitter ; reconnexion auto sur reboot/crash ; '
         'DTR/RTS jamais touches.\n')

    line = ''
    while True:
        port = args.port or detect_port()
        if not port:
            emit('[monitor] %s aucun port ESP detecte, attente...\n' % ts())
            time.sleep(1.0)
            continue
        try:
            s = open_port(port, args.baud)
        except Exception as e:
            emit('[monitor] %s ouverture %s impossible (%s), retry...\n'
                 % (ts(), port, e))
            time.sleep(1.0)
            continue
        emit('[monitor] %s connecte a %s @%d\n' % (ts(), port, args.baud))
        try:
            while True:
                data = s.read(1024)
                if not data:
                    continue
                text = data.decode('utf-8', 'replace')
                for ch in text:
                    if ch == '\n':
                        emit('%s %s\n' % (ts(), line))
                        line = ''
                    elif ch != '\r':
                        line += ch
        except (serial.SerialException, OSError) as e:
            if line:
                emit('%s %s\n' % (ts(), line))
                line = ''
            emit('[monitor] %s port perdu (%s) — reboot/crash/re-enumeration, '
                 'reconnexion...\n' % (ts(), e))
            try:
                s.close()
            except Exception:
                pass
            time.sleep(0.5)


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print('\n[monitor] arret.')
