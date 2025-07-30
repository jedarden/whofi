from setuptools import setup

with open("README.md", "r") as fh:
    long_description = fh.read()

classifiers = [
    'Development Status :: 1 - Planning',
    'Intended Audience :: Education',
    'Operating System :: OS Independent',
    'License :: OSI Approved :: MIT License',
    'Programming Language :: Python :: 3',
    'Programming Language :: Python :: 3.8',
]
 
setup(
    name='csiparser',
    version='0.1.0',
    description='A Python Package to Parse ESP32 Wi-Fi CSI Data',
    py_modules=["csiparser"],
    package_dir={'': 'src'},
    long_description=long_description,
    long_description_content_type="text/markdown",
    install_requires = [
        "numpy ~= 1.19.5",
        "pandas ~= 1.2.3",
    ],
    extras_require = {
        "dev": [
            "pytest>=3.7"
        ],
    },
    url="https://github.com/RikeshMMM/ESP32-CSI-Python-Parser",
    author="Rikesh Makwana",
    author_email="rikesh.makwana@gmail.com",
)