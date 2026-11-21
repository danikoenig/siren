#!/usr/bin/env python3
"""Builds the EN and DE manuals (PDF). Usage: make_manual.py <version> <screenshot> <outdir>"""
import sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import lib
from content import CONTENT
lib.main(CONTENT)
