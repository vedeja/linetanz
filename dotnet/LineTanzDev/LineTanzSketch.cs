using System.Net.Sockets;

public partial class LineTanzSketch
{
    private readonly string host = "192.168.0.161";
    private readonly int port = 50001;
    private Socket socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);

    private byte seqNo;

    private Initializer initializer = new();
    private Inbox inbox = new();
    private DateTime lastKeepAliveSent;
    private bool isMuted;

    public void Setup()
    {
        initializer.RegisterRequestSender(SendRequest);
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

    private byte GetNextSeqNo() => ++seqNo;

    private void KeepAlive()
    {
        var now = DateTime.Now;
        if (now - lastKeepAliveSent > TimeSpan.FromMilliseconds(3000))
        {
            lastKeepAliveSent = now;
            SendRequest(Command.KeepAlive, []);
        }
    }

    private void Receive()
    {
        if (socket.Available > 0)
        {
            var buffer = new byte[socket.Available];
            var bytesRead = socket.Receive(buffer, SocketFlags.None);
            if (bytesRead > 0)
            {
                inbox.Receive(buffer[0..bytesRead]);
            }
        }
    } 

    private void HandleIncomingMessages()
    {
        HandleIncomingRequests();
        HandleIncomingResponses();
        HandleIncomingBroadcasts();
        // TODO: handle error messages?
    }

    private void HandleIncomingRequests()
    {
        foreach (var seqNo in inbox.RequestSequenceNumbers)
        {
            var incomingRequest = inbox.Get(seqNo);

            switch (incomingRequest.Command)
            {
                case Command.Handshake:
                    {
                        var body = new byte[] { 0x10, 0x40, 0, 0, 0, 0, 0, 0 };
                        SendResponse(Command.Handshake, incomingRequest.SequenceNumber, body);
                        break;
                    }

                case Command.Info:
                    {
                        var body = new byte[] { 0, 0, 0, 0x02, 0, 0, 0x40, 0 };
                        SendResponse(Command.Info, incomingRequest.SequenceNumber, body);
                        break;
                    }

                case Command.MessageSizeControl:
                    {
                        var body = incomingRequest.Body[0..4];
                        SendResponse(Command.MessageSizeControl, incomingRequest.SequenceNumber, body);
                        break;
                    }

                default:
                    throw new NotImplementedException($"Un-answered mixer request: {incomingRequest.Command}");
            }
            inbox.Remove(incomingRequest);
        }
    }

    private void HandleIncomingResponses()
    {
        foreach (var seqNo in inbox.ResponseSequenceNumbers)
        {
            var incomingResponse = inbox.Get(seqNo);
            var isHandled = initializer.HandleIncomingResponse(incomingResponse);

            if (isHandled)
            {
                inbox.Remove(incomingResponse);
            }
        }
    }

    private void HandleIncomingBroadcasts()
    {
        foreach (var seqNo in inbox.BroadcastSequenceNumbers)
        {
            var incomingBroadcast = inbox.Get(seqNo);

            //TODO: Handle broadcast message
        }
    }

    private byte Send(MessageType type, Command command, byte[] body)
    {
        var seqNo = GetNextSeqNo();
        Send(new DlMessage(type, command, seqNo, body));
        return seqNo;
    }

    private byte SendRequest(Command command, byte[] body)
    {
        return Send(MessageType.Request, command, body);
    }

    private void SendResponse(Command command, byte seqNo, byte[] body)
    {
        Send(MessageType.Response, command, seqNo, body);
    }

    private void Send(MessageType type, Command command, byte seqNo, byte[] body)
    {
        Send(new DlMessage(type, command, seqNo, body));
    }

    private void Send(DlMessage message)
    {
        var data = Serializer.Serialize(message);
        Console.WriteLine($"OUT <- {message}");
        socket.Send(data);
    }

    public void ToggleMute()
    {
        isMuted = !isMuted;
        Console.WriteLine($"Mute: {isMuted}");
        
        var state = (byte)(isMuted ? 0x01 : 0x00);
        var body = new byte[] { 0x00, 0x00, 0x13, 0x00, 0x00, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00, state };
        

        // 13 67 is the channel number for FX1 input
        body[3] = 0x67;
        SendRequest(Command.ChannelValues, body);

        // 13 C9 is the channel number for FX2 input
        body[3] = 0xC9;
        SendRequest(Command.ChannelValues, body);
    }
}