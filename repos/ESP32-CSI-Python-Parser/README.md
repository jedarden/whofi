# ESP32 Wi-Fi CSI Python Parser

<img src="https://raw.githubusercontent.com/RikeshMMM/ESP32-CSI-Python-Parser/main/examples/images/ESP32_walking_activity.png" width="500px">

This is a Python parser for ESP32 Wi-Fi Channel State Information (CSI) based on the ESP32 CSI specification.

This project uses [ESP CSI Toolkit](https://stevenmhernandez.github.io/ESP32-CSI-Tool/) created by Hernandez and Bulut.

## Installation

Run the following to install:

```python
pip install csiparser
```

## Usage

```python
# Import ESP32 CSI parser
import csiparser

# Parse and filter CSI data
example_csi = (
    csiparser.ESP32("../esp32_dataset/example_csi.csv")
             .filter_by_sig_mode(1)
             .get_csi()
             .remove_null_subcarriers()
             .get_amplitude_from_csi()
             .get_phase_from_csi()
)

# Retrieve example amplitude
example_amplitude = example_csi.amplitude

# Retrieve example phase 
example_phase = example_csi.phase
```
Further the amplitude and phase information can be plotted to visualize the distortion in amplitude and phase shift as follows:

![Example Amplitude and Phase Graph](https://raw.githubusercontent.com/RikeshMMM/ESP32-CSI-Python-Parser/main/examples/images/example_amplitude_and_phase_graph.png)

_See [Examples](./examples) directory for full example._

## Citation

[Cite this Tool with BibTeX](https://raw.githubusercontent.com/RikeshMMM/ESP32-CSI-Python-Parser/main/docs/bibtex/Touchless-Biometric-User-Authentication-Using-ESP32-WiFi-Module-Citation.bib)

## License
Distributed under the MIT License. See LICENSE for more information.
