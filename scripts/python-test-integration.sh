#!/bin/bash

set -e;

if [ -z "$GOOGLE_APPLICATION_CREDENTIALS" ]; then
   echo "Please specify a path to your Google credentials file via GOOGLE_APPLICATION_CREDENTIALS";
   exit 1;
fi

if [ -z "$PYTHON_VERSION" ]; then
    PYTHON_VERSION=3.8;
fi

if [ -z "$BUILD_TARGET" ]; then
    BUILD_TARGET=release
fi

echo "Entering python land..."
cd pythonpkgs/

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

# Install dependencies
echo "Installing dependencies"
pip install -r requirements.txt

cd ../../

export PYTHONPATH=pythonpkgs/ducktables
./build/$BUILD_TARGET/test/unittest --test-dir . "[ducktables-integration]"
