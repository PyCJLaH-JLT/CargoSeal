"""Guard against sorting simultaneous VCD changes by voltage."""
import sys, tempfile, unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from read_capture import read_vcd, i2c

class CaptureTest(unittest.TestCase):
    def test_last_transition_at_one_timestamp_wins(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "edge.vcd"
            source.write_text("$timescale 1ns $end\n$var wire 1 ! SDA $end\n"
                              "$enddefinitions $end\n#0\n1!\n#10\n1!\n0!\n#20\n0!\n")
            trace = read_vcd(source)["SDA"]
            self.assertEqual(trace[1], (10, 0))

    def test_observed_who_am_i(self):
        source = Path(__file__).resolve().parents[1] / "docs/captures/i2c-mpu6050-id.vcd"
        traces = read_vcd(source)
        decoded = i2c(traces["SCL"], traces["SDA"])
        self.assertEqual([x["hex"] for x in decoded if x["kind"] == "BYTE"],
                         ["D0", "75", "D1", "68"])
        self.assertEqual([x["ack"] for x in decoded if x["kind"] == "BYTE"],
                         [True, True, True, False])
if __name__ == "__main__":
    unittest.main()
