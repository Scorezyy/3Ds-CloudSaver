FROM python:3.11-slim

WORKDIR /app
COPY server/server.py /app/server.py

RUN mkdir -p /data

EXPOSE 5000

CMD ["python3", "/app/server.py", "--dir", "/data", "--port", "5000"]
