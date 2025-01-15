import java.io.*;
import java.net.Socket;
import java.util.Scanner;

public class Client {
    public static void main(String[] args) {
        Scanner sc=new Scanner(System.in);
        while (true){
            String file=sc.nextLine();
            ClientThread clientThread=new ClientThread(file);
            clientThread.start();
        }

    }
}
class ClientThread extends Thread{
    String filePath;
    Socket socket;
    private static final String SERVER_ADDRESS="127.0.0.1";
    private static final int SERVER_PORT=5118;
    int CHUNK=1024;

    ClientThread(String file){
        this.filePath=file;
        System.out.println(filePath);
    }
    @Override
    public void run() {
        File file = new File(filePath);
        if (!file.exists()||(!filePath.endsWith(".txt")&&!filePath.endsWith(".mp4")&&!filePath.endsWith(".png")&&!filePath.endsWith(".jpg")&&!filePath.endsWith(".jpeg"))) {
            try {
                socket = new Socket(SERVER_ADDRESS, SERVER_PORT);
                System.out.println("Connected to Server");

                BufferedWriter br = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()));
                br.write("ERROR ");
                br.newLine();
                br.flush();
                System.out.println("Error: Invalid file or format");
            }catch (Exception e){
                throw new RuntimeException();
            }
            return;
        } else {
            try {
                socket = new Socket(SERVER_ADDRESS, SERVER_PORT);
                System.out.println("Connected to Server");

                BufferedWriter br = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()));
                br.write("UPLOAD " + filePath);
                br.newLine();
                br.flush();
                BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(socket.getInputStream()));
                String confirmation = bufferedReader.readLine();

                DataOutputStream dataOutputStream = new DataOutputStream(socket.getOutputStream());
                //dataOutputStream.writeUTF(file.getName());
                //dataOutputStream.writeLong(file.length());
                byte[] buffer = new byte[CHUNK];
                int bytes;
                int sent = 0;
                FileInputStream fileInputStream = new FileInputStream(file);
                while ((bytes = fileInputStream.read(buffer)) != -1) {
                    dataOutputStream.write(buffer, 0, bytes);
                    sent += bytes;
                    dataOutputStream.flush();
                }

                System.out.println("File sent to server");
                dataOutputStream.close();
                fileInputStream.close();
                br.close();
                socket.close();
            } catch (Exception e) {
                throw new RuntimeException(e);
            }

        }
    }
}
