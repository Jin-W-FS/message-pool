#!/usr/bin/python

from time import sleep
from sys import stdout, argv
from random import random, randint

if __name__ == "__main__":
    sleep_max = 0.0
    if len(argv) > 1:
        sleep_max = float(argv[1])
    while True:
        print "%03x" % randint(0, 0xfff)
        stdout.flush()
        sleep(random() * sleep_max)
