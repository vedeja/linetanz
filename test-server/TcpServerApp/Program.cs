using System;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

class Program
{
    private const int Port = 50001;

    static async Task Main(string[] args)
    {
        Console.WriteLine($"Starting TCP server on port {Port}...");

        TcpListener listener = new TcpListener(IPAddress.Any, Port);
        listener.Start();

        while (true)
        {
            Console.WriteLine("Waiting for a connection...");
            TcpClient client = await listener.AcceptTcpClientAsync();
            Console.WriteLine("Client connected.");

            _ = Task.Run(() => HandleClient(client));
        }
    }

    private static async Task HandleClient(TcpClient client)
    {
        NetworkStream stream = client.GetStream();
        byte sequenceNumber = 1;

        while (client.Connected)
        {
            sequenceNumber++;
            if (sequenceNumber > 255)
            {
                sequenceNumber = 1;
            }

            var body = new byte[] { 0x10, 0x40, 0, 0, 0, 0, 0, 0};

            var bodyChunkCount1 = body.Length / 4;
            var bodyChunkCount2 = 0;
            var messageType = 0x00; //Request
            var command = 0x03; //Handshake
            var headerChecksum = 0xffff - (0xAB + sequenceNumber + bodyChunkCount2 + bodyChunkCount1 + messageType + command);
            var hcs = BitConverter.GetBytes(headerChecksum);
            var bodyChecksum = 0xfffffff - (uint)Enumerable.Sum(body, x => (uint)x);
            var bcs = BitConverter.GetBytes(bodyChecksum);

            var message = new byte[] {
                0xAB,
                sequenceNumber,
                (byte)bodyChunkCount2,
                (byte)bodyChunkCount1,
                (byte)messageType,
                (byte)command,
                hcs[1],
                hcs[0],
                body[0],
                body[1],
                body[2],
                body[3],
                body[4],
                body[5],
                body[6],
                body[7],
                bcs[3],
                bcs[2],
                bcs[1],
                bcs[0]
            };
            
            // Actual first message from the mixer: (keep alive request, seq 1)
            message = new byte[] { 0xAB, 0x01, 0x00, 0x00, 0x00, 0x01, 0xFF, 0x52 };

            await stream.WriteAsync(message, 0, message.Length);
            Console.Write("*");
            await Task.Delay(1000); // Send every second
        }

        client.Close();
        Console.WriteLine("Client disconnected.");
    }
}