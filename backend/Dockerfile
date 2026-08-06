FROM python:3.12-slim

WORKDIR /app

COPY requirements.txt .
RUN pip install --no-cache-dir --progress-bar off -r requirements.txt

COPY . .

EXPOSE 8000

# --loop asyncio: uvicorn's default (uvloop, a native/Cython extension) crashed on
# startup in some sandboxed/nested Docker environments during testing. asyncio's pure
# Python loop is a little slower but reliable everywhere, and plenty fast for this app's
# traffic. If you've confirmed uvloop works fine on your host, drop this flag.
CMD ["uvicorn", "app.main:app", "--host", "0.0.0.0", "--port", "8000", "--loop", "asyncio"]
