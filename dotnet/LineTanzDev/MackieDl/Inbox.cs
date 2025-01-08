public class Inbox
{
    public List<DlMessage> Messages = [];

    public List<DlMessage> Requests => Messages.Where(x => x.Type == MessageType.Request).ToList();

    public List<DlMessage> Responses => Messages.Where(x => x.Type == MessageType.Response).ToList();

    public List<DlMessage> Broadcasts => Messages.Where(x => x.Type == MessageType.Broadcast).ToList();

    public List<byte> RequestSequenceNumbers => Requests.Select(x => x.SequenceNumber).ToList();

    public List<byte> ResponseSequenceNumbers => Responses.Select(x => x.SequenceNumber).ToList();

    public List<byte> BroadcastSequenceNumbers => Broadcasts.Select(x => x.SequenceNumber).ToList();

    public void Receive(ReadOnlyMemory<byte> data)
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
            Messages.Add(dlmsg);
            Console.WriteLine($"IN  -> {dlmsg}");
        }
    }

    public DlMessage Get(byte seqNo)
    {
        return Messages.FirstOrDefault(x => x.SequenceNumber == seqNo);
    }

    public void Remove(DlMessage message)
    {
        Messages.Remove(message);
    }
}