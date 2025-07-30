from csiparser import ESP32

csifilepath = "examples/esp32_dataset/example_csi.csv"
csifile = ESP32(csifilepath)

def test_read_file():
    assert csifile.read_file().shape[0] > 0 and csifile.read_file().shape[1] == 27