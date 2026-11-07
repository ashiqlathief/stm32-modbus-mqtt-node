#!/usr/bin/env python3
"""Draws docs/architecture.png (block diagram used in the README). Run: python3 docs/make_diagram.py"""
import os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch

W, H = 17.1, 3.9
fig, ax = plt.subplots(figsize=(W, H), dpi=150)
ax.set_xlim(0, W); ax.set_ylim(0, H); ax.axis("off")

NAVY, GREEN, BLUE, ORANGE, GREY = "#0B2A7A", "#DDF1DD", "#DCE6FA", "#FBE8CF", "#EEEEEE"


def zone(x, w, title):
    ax.add_patch(FancyBboxPatch((x, 0.3), w, 3.3, boxstyle="round,pad=0.02,rounding_size=0.15",
                                fc="#F7F9FE", ec="#9AA8CC", lw=1.2, ls="--"))
    ax.text(x + 0.2, 3.3, title, fontsize=11, color=NAVY, fontweight="bold", va="center")


def block(x, w, title, sub, fc):
    y, h = 1.0, 1.7
    ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.02,rounding_size=0.12",
                                fc=fc, ec=NAVY, lw=1.6))
    ax.text(x + w / 2, y + h * 0.66, title, ha="center", va="center", fontsize=12, fontweight="bold")
    ax.text(x + w / 2, y + h * 0.30, sub, ha="center", va="center", fontsize=9.5, color="#333")


def link(x0, x1, label):
    y = 1.85
    ax.add_patch(FancyArrowPatch((x0, y), (x1, y), arrowstyle="-|>", mutation_scale=16, color=NAVY, lw=1.8))
    ax.text((x0 + x1) / 2, y + 0.18, label, ha="center", va="bottom", fontsize=9.5, color=NAVY)


zone(0.25, 6.9, "Simulated in Renode")
zone(7.5, 9.35, "PC")

block(0.6, 2.0, "Si7021", "temperature\nsensor model", GREY)
block(3.9, 2.9, "STM32F407", "secure bootloader (Ed25519)\n+ signed app: FreeRTOS,\nI2C driver, Modbus slave", GREEN)
block(8.1, 2.0, "Rust gateway", "polls once a second", GREEN)
block(11.4, 2.0, "Mosquitto", "MQTT broker", ORANGE)
block(14.7, 1.8, "Subscriber", "receives JSON", GREY)

link(2.6, 3.9, "I2C")
link(6.8, 8.1, "Modbus\nRTU")
link(10.1, 11.4, "MQTT\nover TLS")
link(13.4, 14.7, "JSON")

out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "architecture.png")
fig.savefig(out, bbox_inches="tight", facecolor="white")
print("wrote", out)
