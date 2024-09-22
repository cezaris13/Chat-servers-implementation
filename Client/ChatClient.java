import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.*;
import java.net.*;
import java.util.HashMap;
import java.util.Map;
import java.util.function.Function;

import java.util.HashMap;
import java.util.Map;
import java.util.function.Function;

import javax.swing.*;

/**
 * Client:

 * 2) If the name is accepted, receives "NAME_OK" from the socket;
 * 3) ... then the client can send messages, which the socket distributes to
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
                "Enter socket IP and port separated by space (ex: 127.0.0.1 20000):",
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
        String socketAddressAndPort = showIPAndPortDialog();
        String[] info = socketAddressAndPort.split("[ ]+");
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

            if (bufferedReaderText.startsWith("SENDNAME")) {
                printWriter.println(showEnterNameDialog());
            } else if (bufferedReaderText.startsWith("VK")) {
                textField.setEditable(true);
            } else if (bufferedReaderText.startsWith("MESSAGE")) {
                messageTextArea.append(bufferedReaderText.substring(7) + "\n");
            } else if (bufferedReaderText.startsWith("FILE")) {
                shouldReceiveFile = true;
            } else if (bufferedReaderText.startsWith("RECEIVEFILE")) {
                receiveFile(bufferedReaderText);
            }
        }
    }

    private void receiveFile(String bufferedReaderText) throws IOException {
        String fileName = bufferedReaderText.substring(12).trim();
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

    public static void main(String[] args) throws Exception {
        ChatClient chatClient = new ChatClient();
        chatClient.frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        chatClient.frame.setVisible(true);
        chatClient.run();
    }
}
