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
    private String commandPath = "../../commands";
    BufferedReader bufferedReader;
    PrintWriter printWriter;
    JFrame frame = new JFrame("Chats");
    JTextField textField = new JTextField(40);
    JTextArea messageTextArea = new JTextArea(8, 40);

    private String port;
    private String ip;

    public ChatClient() {
       initGui();
    }

    public ChatClient(String ip, String port) {
        this.ip = ip;
        this.port = port;
        initGui();
    }

    private void initGui(){
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

    public Map<String, String> loadEnv(String filePath) throws IOException {
        Map<String, String> envVariables = new HashMap<>();

        // Open and read the .env file line by line
        try (BufferedReader reader = new BufferedReader(new FileReader(filePath))) {
            String line;
            while ((line = reader.readLine()) != null) {
                // Skip empty lines and comments
                if (line.trim().isEmpty() || line.startsWith("#")) {
                    continue;
                }

                // Split the line by '=' to separate key and value
                String[] keyValue = line.split("=", 2);
                if (keyValue.length == 2) {
                    String key = keyValue[0].trim();
                    String value = keyValue[1].trim();
                    envVariables.put(key, value);
                }
            }
        }

        return envVariables;
    }

    private void run() throws IOException {
        Map<String,String> commands = loadEnv(commandPath);
        // Logging in and UI inicialization
        Socket socket;
        if (ip == null || port == null){
            String socketAddressAndPort = showIPAndPortDialog();
            String[] info = socketAddressAndPort.split("[ ]+");
            socket = new Socket(info[0], Integer.parseInt(info[1]));
        }
        else {
            socket = new Socket(ip, Integer.parseInt(port));
        }
        bufferedReader = new BufferedReader(new InputStreamReader(
                socket.getInputStream()));
        printWriter = new PrintWriter(socket.getOutputStream(), true);

        // Protocol realization from client side:
        boolean shouldReceiveFile = false;
        while (true) {
            String bufferedReaderText = bufferedReader.readLine();

            System.out.println();

            if (shouldReceiveFile) {
                if (bufferedReaderText.startsWith(commands.get("End")))
                    shouldReceiveFile = false;
                else
                    messageTextArea.append(bufferedReaderText + "\n");
                continue;
            }

            System.out.println("Server ---> Client : '" + bufferedReaderText + "'");
            System.out.println(bufferedReaderText.length());

            if (bufferedReaderText.startsWith(commands.get("SendName"))) {
                printWriter.println(showEnterNameDialog());
            } else if (bufferedReaderText.startsWith(commands.get("NameOk"))) {
                textField.setEditable(true);
            } else if (bufferedReaderText.startsWith(commands.get("Message"))) {
                messageTextArea.append(bufferedReaderText.substring(commands.get("Message").length()) + "\n");
            } else if (bufferedReaderText.startsWith(commands.get("File"))) {
                shouldReceiveFile = true;
            } else if (bufferedReaderText.startsWith(commands.get("ReceiveFile"))) {
                receiveFile(bufferedReaderText);
            }
        }
    }

    private void receiveFile(String bufferedReaderText) throws IOException {
        System.out.println(bufferedReaderText);
        Map<String,String> commands = loadEnv(commandPath);

        String fileName = bufferedReaderText.substring(commands.get("ReceiveFile").length()).trim();
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
        printWriter.flush();
        stream.close();
    }

    public static void main(String[] args) throws Exception {
        ChatClient chatClient = args.length == 2 ? new ChatClient(args[0],args[1]) : new ChatClient();
        chatClient.frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        chatClient.frame.setVisible(true);
        chatClient.run();
    }
}
