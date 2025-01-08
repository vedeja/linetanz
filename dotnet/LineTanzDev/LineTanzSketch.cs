using System.Net.Sockets;

public partial class LineTanzSketch
{
    private readonly string host = "192.168.0.161";
    private readonly int port = 50001;
    private Socket socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);

    private byte seqNo;

    private Initializer initializer = new();
    private DateTime lastKeepAliveSent;
    private List<DlMessage> incomingMessages = [];

    public void Setup()
    {
        initializer.RegisterSender(Send);
        initializer.RegisterSeqNoGenerator(GetNextSeqNo);
        socket.Connect(host, port);
        seqNo = 0;
    }

    public void Loop()
    {
        if (initializer.Initialize())
        {
            KeepAlive();
        }

        Receive();
        HandleIncomingMessages();

        Thread.Sleep(100);
    }

    private int GetNextSeqNo() => ++seqNo;

    private void KeepAlive()
    {
        var now = DateTime.Now;
        if (now - lastKeepAliveSent > TimeSpan.FromMilliseconds(3000))
        {
            lastKeepAliveSent = now;
            Send(new DlMessage(MessageType.Request, Command.KeepAlive, GetNextSeqNo(), []));
        }
    }

    private void HandleIncomingMessages()
    {
        HandleIncomingRequests();
        HandleIncomingResponses();

        incomingMessages.Clear();
    }

    private void HandleIncomingRequests()
    {
        foreach (var incomingRequest in incomingMessages.Where(x => x.Type == MessageType.Request))
        {
            if (incomingRequest.Type == MessageType.Request)
            {
                switch (incomingRequest.Command)
                {
                    case Command.Handshake:
                        {
                            var body = new byte[] { 0x10, 0x40, 0, 0, 0, 0, 0, 0 };
                            Send(new DlMessage(MessageType.Response, Command.Handshake, incomingRequest.SequenceNumber, body));
                            break;
                        }

                    case Command.Info:
                        {
                            var body = new byte[] { 0, 0, 0, 0x02, 0, 0, 0x40, 0 };
                            Send(new DlMessage(MessageType.Response, Command.Info, incomingRequest.SequenceNumber, body));
                            break;
                        }

                    case Command.MessageSizeControl:
                        {
                            var body = incomingRequest.Body[0..4];
                            Send(new DlMessage(MessageType.Response, Command.MessageSizeControl, incomingRequest.SequenceNumber, body));
                            break;
                        }

                    default:
                        throw new NotImplementedException($"Un-answered mixer request: {incomingRequest.Command}");
                }
            }
        }
    }

    private void HandleIncomingResponses()
    {
        foreach (var incomingResponse in incomingMessages.Where(x => x.Type == MessageType.Response))
        {
            initializer.HandleIncomingResponse(incomingResponse);
        }
    }
   
    private void Send(DlMessage message)
    {
        var data = Serializer.Serialize(message);
        Console.WriteLine($"OUT <- {message}");
        socket.Send(data);
    }

    private void Receive()
    {
        if (socket.Available > 0)
        {
            var buffer = new byte[socket.Available];
            var bytesRead = socket.Receive(buffer, SocketFlags.None);
            if (bytesRead > 0)
            {
                ProcessData(buffer[0..bytesRead]);
            }
        }
    }

    private void ProcessData(ReadOnlyMemory<byte> data)
    {
        //Console.WriteLine($"Received {data.Length} bytes");

        var dataArray = data.ToArray();
        var messageStartIndexes = new List<int>();

        var currentIndex = -1;

        while (true)
        {
            var nextStart = Array.IndexOf(dataArray, DlMessage.MESSAGE_START, ++currentIndex);

            if (nextStart == -1)
            {
                // START Not found (probably because we processed the only or last message)
                break;
            }

            messageStartIndexes.Add(nextStart);
            currentIndex = nextStart;
        }

        //Console.WriteLine($"Received {messageStartIndexes.Count} messages");

        // Handle each individual message
        for (int i = 0; i < messageStartIndexes.Count; i++)
        {
            int startIndex = messageStartIndexes[i];
            var isLastItem = i == messageStartIndexes.Count - 1;

            var endIndex = isLastItem ? dataArray.Length - 1 : messageStartIndexes[i + 1];
            var messageData = dataArray[startIndex..(endIndex + 1)];

            var dlmsg = DlMessage.FromData(messageData);
            incomingMessages.Add(dlmsg);
            Console.WriteLine($"IN  -> {dlmsg}");
        }
    }
}