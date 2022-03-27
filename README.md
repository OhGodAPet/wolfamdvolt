# wolfamdvolt

## Background

I wanted to have low-level fine-grained control of power delivery on my AMD GPUs, and I was unwilling to actually touch the cards for it, in part because I cannot solder for shit. This, at first glance, is impossible - the I2C bus with the VRM controller is simply not connected to anything off the GPU. However... the GPU itself will happily listen/speak on the I2C bus on your behalf if you know how to ask it...

The driver controls and configures the GPU via registers accessible over PCIe. Basically, a PCIe BAR (Base Address Register) is a memory-mapped region where all reads query the device, and all writes are sent to it. Functionality is exposed via 32-bit registers (rarely, they can be larger.) With knowledge of the internal registers, one can do MANY fun things with an AMD GPU, and one of them is using the internal I2C buses.

I bought one of every Polaris card with a unique VRM controller (this was back before AMD mandated the use of the IR36217 with Vega's debut) to make this project. I reverse engineered the internal GPU control registers (accessible over PCIe - this is how the driver controls it) responsible for giving access to the GPU's internal I2C bus, which isn't connected to any other input/output. On this bus is the VRM controller (there's sometimes other goodies like fan controllers, temp sensors, RGB light controls.)

From there, we can tweak the very lowest level details of power delivery - not just the voltage itself, but current/voltage/temp limits, load-line calibration, and more. I believe it will work on any Polaris AMD GPU, but the specific chips it supports are the Infineon/International Rectifier IR356XX and IR35217, the ON Semi NCP81022, the Realtek RT8894A, and the uPI Semiconductor uP1801 and uP9505P.

## Features

- Supports temp monitoring on IR3567B
- Supports voltage getting and setting on IR3567B, NCP81022, RT8894A, uP1801, and uP9505
- Supports voltage getting on IR35217 (RX Vega 56/64)

## Sample output from an RX Vega 64 with no arguments:

```
[root@artica ~]# wolfamdvolt -i 0
wolfamdvolt v0.95
GPU 0:
    Number of VRMs: 2
    VRM 0: IR35217
        Number of outputs: 2
        Output 0:
            Voltage: 1.1000
            Offset: 0.0000
        Output 1:
            Voltage: 1.3500
            Offset: 0.0000
    VRM 1: uP1801
        Number of outputs: 1
        Output 0:
            Voltage: 1.2500
```

## Donations accepted!

- ETH: `0xCED1D126181874124A5Cb05563A4A8879D1CeC85`
