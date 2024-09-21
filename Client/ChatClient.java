import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.*;
import java.net.*;
import javax.swing.*;

/**
 * Client:
 * 1) Receives the message "SEND_NAME" from the server (repeatedly, until a
 * valid name is provided);
 * 2) If the name is accepted, receives "NAME_OK" from the server;
 * 3) ... then the client can send messages, which the server distributes to
 * everyone.
 */
public class ChatClient {
    BufferedReader bufferedReader;
    PrintWriter printWriter;
    JFrame frame = new JFrame("Chats");
    JTextField textField = new JTextField(40);
    JTextArea messageTextArea = new JTextArea(8, 40);

    public ChatClient() {
        // GUI:
        textField.setEditable(false); // text field should not be editable until user logs in
        messageTextArea.setEditable(false); // message area should not be editable until user logs in
        frame.getContentPane().add(textField, "North");
        frame.getContentPane().add(new JScrollPane(messageTextArea), "Center");
        frame.pack();

        // onActionListener
        textField.addActionListener(e -> {
            printWriter.println(textField.getText());
            textField.setText("");
        });
    }

    private String showIPAndPortDialog() {
        return JOptionPane.showInputDialog(
                frame,
                "Enter server IP and port:",
                "IP and port",
                JOptionPane.QUESTION_MESSAGE);
    }

    private String showEnterNameDialog() {
        return JOptionPane.showInputDialog(
                frame,
                "Enter your name:",
                "Name",
                JOptionPane.PLAIN_MESSAGE);
    }

    private void run() throws IOException {
        // Logging in and UI inicialization
        String serverAddressAndPort = showIPAndPortDialog();
        String[] info = serverAddressAndPort.split("[ ]+");
        Socket socket = new Socket(info[0], Integer.parseInt(info[1]));
        bufferedReader = new BufferedReader(new InputStreamReader(
                socket.getInputStream()));
        printWriter = new PrintWriter(socket.getOutputStream(), true);

        // Protocol realization from client side:
        boolean shouldReceiveFile = false;
        while (true) {
            String bufferedReaderText = bufferedReader.readLine();

            System.out.println();

            if (shouldReceiveFile) {
                if (bufferedReaderText.startsWith("%END%"))
                    shouldReceiveFile = false;
                else
                    messageTextArea.append(bufferedReaderText + "\n");
                continue;
            }

            System.out.println("Server ---> Client : '" + bufferedReaderText + "'");
            System.out.println(bufferedReaderText.length());

            switch (bufferedReaderText) {
                case bufferedReaderText.startsWith("SENDNAME"):
                    printWriter.println(showEnterNameDialog());
                    break;
                case bufferedReaderText.startsWith("VK"):
                    textField.setEditable(true);
                    break;
                case bufferedReaderText.startsWith("PRANESIMAS"):
                    messageTextArea.append(bufferedReaderText.substring(10) + "\n");
                    break;
                case bufferedReaderText.startsWith("FAILAS"):
                    shouldReceiveFile = true;
                    break;
                case bufferedReaderText.startsWith("GAUTIFAILA"):
                    receiveFile(bufferedReaderText);
                    break;
                default:
                    break;
            }
        }

        private void receiveFile(String bufferedReaderText){
            String fileName = bufferedReaderText.substring(10).trim();
            String strLine;
            File file = new File(fileName);
            boolean isFileCreated = file.createNewFile();

            FileInputStream stream = new FileInputStream(fileName);

            BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(stream));

            while ((strLine = bufferedReader.readLine()) != null) {
                printWriter.println(strLine);
                System.out.println(strLine);
                printWriter.flush();
            }
            System.out.println("that's it");
            printWriter.println("%END%\0");
            printWriter.flush();
            stream.close();
        }
    }

    public static void main(String[] args) throws Exception {
        ChatClient chatClient = new ChatClient();
        chatClient.frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        chatClient.frame.setVisible(true);
        chatClient.run();
    }
}
