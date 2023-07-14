#!/bin/bash

set -e;

echo "Entering python land..."
cd pythonpkgs/

if [ -z "$PYTHON_VERSION" ]; then
    PYTHON_VERSION=3.8;
fi

echo "Checking that we have a python..."
which python$PYTHON_VERSION

# Create a virtual environment
echo "Creating our virtual environment"
python$PYTHON_VERSION -m venv env-py$PYTHON_VERSION
echo "Sourcing the env"
source env-py$PYTHON_VERSION/bin/activate

cd ducktables;

# Upgrade pip
echo "Upgrading pip..."
pip install --upgrade pip
pip install wheel

# Install dependencies
echo "Installing dependencies"
pip install -r requirements.txt

# Run unit tests
echo "Running tests..."
python -m unittest discover -s tests/ducktables/

# Build the package
echo "Creating package..."
python setup.py sdist bdist_wheel

# Deactivate the virtual environment
deactivate

# Clean up
rm -rf myenv
