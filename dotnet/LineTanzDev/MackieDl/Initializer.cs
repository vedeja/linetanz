
public class Initializer
{
    private bool isInitialized;
    private int initializationStatus;
    private Action<DlMessage>? send;
    private Func<int>? seqNoGenerator;
    private List<DlMessage> outgoingInitMessages = [];

    public bool Initialize()
    {
        if (!isInitialized)
        {
            Proceed();
        }

        return isInitialized;
    }

    public void RegisterSender(Action<DlMessage> send)
    {
        this.send = send;
    }

    internal void RegisterSeqNoGenerator(Func<int> seqNoGenerator)
    {
        this.seqNoGenerator = seqNoGenerator;
    }

    public void HandleIncomingResponse(DlMessage incomingResponse)
    {
        if (!isInitialized)
        {
            var request = outgoingInitMessages.SingleOrDefault(x => x.SequenceNumber == incomingResponse.SequenceNumber && x.Command == incomingResponse.Command);
            if (request != null)
            {
                switch (initializationStatus)
                {
                    case 1:
                        initializationStatus = 2;
                        break;

                    case 3:
                        initializationStatus = 4;
                        break;

                    case 5:
                        initializationStatus = 6;
                        break;

                    case 7:
                        initializationStatus = 8;
                        break;

                    default:
                        break;
                }
                outgoingInitMessages.Remove(request);
            }
        }
    }


    private void Proceed()
    {
        switch (initializationStatus)
        {
            case 0:
                Send(new DlMessage(MessageType.Request, Command.KeepAlive, seqNoGenerator(), []));
                initializationStatus = 1;
                break;

            case 2:
                Send(new DlMessage(MessageType.Request, Command.MessageSizeControl, seqNoGenerator(), [0, 0, 0, 0, 0, 0, 0, 0]));
                initializationStatus = 3;
                break;

            case 4:
                Send(new DlMessage(MessageType.Request, Command.Handshake, seqNoGenerator(), []));
                initializationStatus = 5;
                break;

            case 6:
                Send(new DlMessage(MessageType.Request, Command.Info, seqNoGenerator(), [0, 0, 0, 0x02]));
                initializationStatus = 7;
                break;

            case 8:
                isInitialized = true;
                break;

            default:
                break;
        }

        // TODO: Command.MeterSelection?
        // TODO: Command.PeriodicMeters?
    }

    private void Send(DlMessage message)
    {
        outgoingInitMessages.Add(message);
        send(message);
    }
}