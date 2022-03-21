import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.*;
import java.net.*;

import javax.swing.*;

/**
 * Klientas:
 * 1) gauna  pranesima "ATSIUSK_VARDA" is serverio (tol, kol nebus tinkamo vardo);
 * 2) jeigu vardas priimtas, gauna is serverio "VARDAS_OK";
 * 3) ... tada gali rasyti pranesimus, kuriuos serveris dalina visiems
 */
public class PokalbiuKlientas {

    BufferedReader ivestis;
    PrintWriter isvestis;
    JFrame langas = new JFrame("Pokalbiai");
    JTextField tekstoLaukelis = new JTextField(40);
    JTextArea pranesimuSritis = new JTextArea(8, 40);

    public PokalbiuKlientas() {

        // GUI dizainas:
        tekstoLaukelis.setEditable(false); // kol negavome neprisijugeme
        pranesimuSritis.setEditable(false); // kol negavome neprisijugeme
        langas.getContentPane().add(tekstoLaukelis, "North");
        langas.getContentPane().add(new JScrollPane(pranesimuSritis), "Center");
        langas.pack();

        // Dorokliai
        tekstoLaukelis.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
                isvestis.println(tekstoLaukelis.getText());
                tekstoLaukelis.setText("");
            }
        });
    }

    // Kur yra serveris?
    private String koksServerioAdresasIrPortas() {
        return JOptionPane.showInputDialog(
            langas,
            "Serverio IP ir portas?",
            "Klausimas",
            JOptionPane.QUESTION_MESSAGE);
    }

    // Koks vardas? 
    private String koksVardas() {
        return JOptionPane.showInputDialog(
            langas,
            "Pasirink varda:",
            "Vardas...",
            JOptionPane.PLAIN_MESSAGE);
    }

    /**
     * Prisijungiame prie serverio ir bandome dalyvauti pokalbiuose
     */
    private void run() throws IOException {

        // Prisijungiame ir inicializuojame I/O...
        String serverioAdresasIrPortas = koksServerioAdresasIrPortas();
        String[] info = serverioAdresasIrPortas.split("[ ]+");     
        Socket soketas = new Socket(info[0], Integer.parseInt(info[1]));
        ivestis = new BufferedReader(new InputStreamReader(
            soketas.getInputStream()));
        isvestis = new PrintWriter(soketas.getOutputStream(), true);

        // Realizuojame protokola is kliento puses:
        boolean gautiFaila = false;
        while (true) {
            String tekstas = ivestis.readLine();

            System.out.println();
            if(gautiFaila == false){
                System.out.println("Serveris ---> Klientas : \'" + tekstas+"\'");
                System.out.println(tekstas.length());
                if (tekstas.startsWith("ATSIUSKVARDA")) {
                    isvestis.println(koksVardas());
                }
                else if (tekstas.startsWith("VK")) {
                    tekstoLaukelis.setEditable(true);
                }
                else if (tekstas.startsWith("PRANESIMAS")) {
                    pranesimuSritis.append(tekstas.substring(10) + "\n");
                }
                else if(tekstas.startsWith("GAUTIFAILA")){
                    String failas = tekstas.substring(10).trim();
                    String strLine;
                    File yourFile = new File(failas);
                    yourFile.createNewFile();

                    FileInputStream fstream = new FileInputStream(failas);

                    BufferedReader br = new BufferedReader(new InputStreamReader(fstream));

                    while ((strLine = br.readLine()) != null){
                        isvestis.println(strLine);
                        System.out.println(strLine);
                        isvestis.flush();
                    }  
                    System.out.println("thatsit");
                    isvestis.println("%END%\0");
                    isvestis.flush();
                    fstream.close();
                }
                else if(tekstas.startsWith("FAILAS")){
                    gautiFaila = true;
                }
            }
            else{
                if(tekstas.startsWith("%END%")){
                    gautiFaila=false;
                }
                else{
                    pranesimuSritis.append(tekstas + "\n");
                }
            }
        }
    }

    /**
     * Konstruojame klienta...
     */
    public static void main(String[] args) throws Exception {
        PokalbiuKlientas klientas = new PokalbiuKlientas();
        klientas.langas.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        klientas.langas.setVisible(true);
        klientas.run();
    }
}
