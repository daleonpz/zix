import asyncio
import sys
from bleak import BleakClient

CERT_SERVICE_UUID = "12345678-1234-5678-1234-56789abcdef0"
DEVICE_CERTIFICATE_UUID = "12345678-1234-5678-1234-56789abcdef1"
CENTRAL_CERTIFICATE_UUID = "12345678-1234-5678-1234-56789abcdef2"

HOST_CERTIFICATE = b"""-----BEGIN CERTIFICATE-----
miibiJanbGKQHKIg9W0baqefaaocaq8amiibcGkcaqeaZvHe
-----END CERTIFICATE-----
"""


async def exchange_certs(address):
    async with BleakClient(address) as client:
        # Read device certificate from the device
        device_cert = await client.read_gatt_char(CENTRAL_CERTIFICATE_UUID)
        print("Device certificate read from device.")
        print("Device Certificate:\n%s", device_cert.decode())

        # Write host certificate to the device
        await client.write_gatt_char(DEVICE_CERTIFICATE_UUID, HOST_CERTIFICATE)
        print("Host certificate written to device.")


if __name__ == "__main__":
    ## device address is the input argument
    if len(sys.argv) != 2:
        print("Usage: python central.py <device_address>")
        sys.exit(1)

    device_address = sys.argv[1]
    asyncio.run(exchange_certs(device_address))
