FROM python:3.10-slim

WORKDIR /app

# Install dependencies
RUN pip install --no-cache-dir \
    paho-mqtt \
    numpy \
    scipy \
    scikit-learn \
    pandas \
    asyncio-mqtt \
    websockets \
    pyyaml

# Copy application files
COPY csi_processor.py /app/
COPY config.yaml /app/

# Expose WebSocket port for real-time updates
EXPOSE 8765

CMD ["python", "-u", "csi_processor.py"]