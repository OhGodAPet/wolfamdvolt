# wolfamdvolt

# Supports temp monitoring on IR3567B
# Supports voltage getting and setting on IR3567B, NCP81022, RT8894A, uP1801, and uP9505
# Supports voltage getting on IR35217

Sample output from an RX Vega 64 with no arguments:

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

