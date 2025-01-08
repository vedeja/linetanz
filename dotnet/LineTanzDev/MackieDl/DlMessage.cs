using System.Text;

public class DlMessage
{
    /// <summary>
    /// Begins all Mackie DL messages (decimal 171)
    /// </summary>
    public const byte MESSAGE_START = 0xAB;

    public int SequenceNumber { get; set; }
    public MessageType Type { get; set; }
    public Command Command { get; set; }
    public byte[] Body { get; set; }

    public DlMessage(MessageType messageType, Command command, int sequenceNumber, byte[] body)
    {
        Type = messageType;
        Command = command;
        SequenceNumber = sequenceNumber;
        Body = body;
    }

    public static DlMessage FromData(byte[] data)
    {
        // TODO: validate header and body checksums, and handle invalid messages by throwing or returning some special error object 
        if (data[0] != MESSAGE_START)
        {
            throw new Exception("Message did not start with 0xAB");
        }

        var seqno = data[1];
        var type = (MessageType)data[4];
        var command = (Command)data[5];

        var bodyChunkCount1 = data[3];
        var bodyLength = bodyChunkCount1 * 4;
        var bodyOffset = 8;
        byte[] body = data[bodyOffset..(bodyOffset + bodyLength)];


        return new DlMessage(type, command, seqno, body);

    }

    public override string ToString()
    {
        var paddedSequenceNumber = SequenceNumber.ToString("D3");
        var bodyString = Body.Length > 0 ? $"\t{PrintByteArray(Body)} {Body.Length} bytes" : "";
        return $"{paddedSequenceNumber} {Type.ToString().ToUpper()}\t{Command}{bodyString}";
    }

    private string PrintByteArray(byte[] bytes)
    {
        bytes = Reverse(bytes);

        var sb = new StringBuilder("{ ");
        for (int i = 0; i < bytes.Length; i++)
        {
            byte b = bytes[i];
            //sb.Append(b);
            sb.Append(BitConverter.ToString([b]));

            // if (i < (bytes.Length - 1))
            // {
            //     sb.Append(", ");
            // }

            sb.Append(" ");
        }
        sb.Append("}");
        return sb.ToString();
    }

    private byte[] Reverse(byte[] data)
    {
        byte[] newData = new byte[data.Length];
        for (int i = 0; i < data.Length; i += 4)
        {
            newData[i] = data[i + 3];
            newData[i + 1] = data[i + 2];
            newData[i + 2] = data[i + 1];
            newData[i + 3] = data[i];
        }
        return newData;
    }
}