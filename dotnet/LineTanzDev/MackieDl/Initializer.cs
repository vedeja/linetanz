
public class Initializer
{
    private bool isInitialized;
    private int initializationStatus;
    private Func<Command, byte[], byte>? sendRequestAndGetSeqNo;
    
    private Dictionary<byte, Command> outgoingInitMessages = [];

    public bool Initialize()
    {
        if (!isInitialized)
        {
            Proceed();
        }

        return isInitialized;
    }

    public void RegisterRequestSender(Func<Command, byte[], byte> sendRequestAndGetSeqNo)
    {
        this.sendRequestAndGetSeqNo = sendRequestAndGetSeqNo;
    }

    public bool HandleIncomingResponse(DlMessage incomingResponse)
    {
        if (!isInitialized)
        {
            if(outgoingInitMessages.TryGetValue(incomingResponse.SequenceNumber, out var request) && request == incomingResponse.Command)
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
                outgoingInitMessages.Remove(incomingResponse.SequenceNumber);
                return true;
            }
        }
        return false;
    }


    private void Proceed()
    {
        switch (initializationStatus)
        {
            case 0:                
                SendRequest(Command.KeepAlive, []);
                initializationStatus = 1;
                break;

            case 2:                
                SendRequest(Command.MessageSizeControl, [0, 0, 0, 0, 0, 0, 0, 0]);
                initializationStatus = 3;
                break;

            case 4:                
                SendRequest(Command.Handshake, []);
                initializationStatus = 5;
                break;

            case 6:                
                SendRequest(Command.Info, [0, 0, 0, 0x02]);
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

    private void SendRequest(Command command, byte[] body)      
    {
        var seqNo = sendRequestAndGetSeqNo(command, body);
        outgoingInitMessages.Add(seqNo, command);
    }
}