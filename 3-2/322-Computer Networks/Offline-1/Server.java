import java.io.*;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.URLConnection;
import java.util.Date;
import java.util.Objects;

public class Server {
    static int port=5118;
    public static void main(String[] args) {
        try {
            ServerSocket serverSocket = new ServerSocket(port, 120);
            //sstem.out.println("Server started");
            //PrintWriter log=new PrintWriter("log.txt");
            String root=new File("").getAbsolutePath();
            File rootDir=new File(root+"/root");
            if(!rootDir.exists()){
                rootDir.mkdir();
            }
            File uploadDir=new File(root+"/root/uploaded");
            if(!uploadDir.exists()){
                uploadDir.mkdir();
            }
            while (true){
                Socket connection=serverSocket.accept();
                //sstem.out.println("Client connected");
                ServerThread serverThread=new ServerThread(connection,root);
                //System.out.println(connection);
                serverThread.start();

            }

        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }
}
class ServerThread extends Thread{
    Socket connection;
    String path;
    private DataOutputStream dataOutputStream=null;
    private int chunk=1024;
    private PrintStream printStream;
    private PrintStream console;
    File file=null;
    FileWriter log;
    ServerThread(Socket connection,String path) throws IOException {
        this.connection=connection;
        this.path=path;
        dataOutputStream=new DataOutputStream(connection.getOutputStream());
        //this.log=new FileWriter("log.txt",true);
        printStream=new PrintStream(new FileOutputStream("log.txt",true));
        console =System.out;
    }
    @Override
    public void run(){
        try {
            BufferedReader in=new BufferedReader(new InputStreamReader(connection.getInputStream()));
            PrintWriter out=new PrintWriter(connection.getOutputStream());
            String request=in.readLine();
            //sstem.out.println("Request "+request);
            System.setOut(printStream);
            System.out.println("HTTP Request from client "+request);
            System.out.println("HTTP Response from server");
            if(request!=null){
//                if(request.startsWith("GET / ")){
//                    sendHomePage(out);
//                }
//                else if(request.startsWith("GET /showText ")){
//                    sendTextPage(out);
//                }
//                else{
//                    sendNotFound(out);
//                }
                //sstem.out.println(request);
                String tokens[]=request.split(" ");
                if(tokens.length>=2){
                    path=new File("").getAbsolutePath();
                    path=path+tokens[1];
                    file=new File(path);
                }

                if(tokens[0].equalsIgnoreCase("UPLOAD")){
                    String fileName=tokens[1];
                    recieveFile(fileName);
                }
                else if(tokens[0].equalsIgnoreCase("Error")){
                    System.setOut(console);
                    System.out.println("Error: Invalid file or format");
                    System.setOut(printStream);
                }
                else if(tokens[1].equalsIgnoreCase("/")){
                    sendHomePage(out);
                }
                else if(!file.exists()){
                    sendNotFound(out);
                    System.setOut(console);
                    System.out.println("Error 404 page not found");
                    System.setOut(printStream);

                }
                else if(file.isDirectory()){
                    sendDirectoryList(out,file);
                }
                else {
                    sendFile(path);
                }
            }
            in.close();
            out.close();
            printStream.close();
            connection.close();
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
    private void sendHomePage(PrintWriter out) throws IOException {
        out.println("HTTP/1.0 200 OK");
        out.println("Server: Java HTTP Server: 1.0");
        out.println("Date: " + new Date());
        out.println("Content-Type: text/html");
        out.println();
        out.println("<html>");
        out.println("<body>");
        out.println("<h1>Welcome to My Web Server</h1>");
        out.println("<a href=\"/root/\">Click here to enter root directory</a>"); // Link to /showText
        out.println("</body>");
        out.println("</html>");
        out.flush();

        System.out.println("HTTP/1.0 200 OK");
        System.out.println("server: Java HTTP Server: 1.0");
        System.out.println("Date: " + new Date());
        System.out.println("Content-Type: text/html");
        System.out.println("");
        System.out.println("<html>");
        System.out.println("<body>");
        System.out.println("<h1>Welcome to My Web Server</h1>");
        System.out.println("<a href=\"/root/\">Click here to enter root directory</a>"); // Link to /showText
        System.out.println("</body>");
        System.out.println("</html>");
    }
    private void sendNotFound(PrintWriter out) throws IOException {
        out.println("HTTP/1.0 404 Not Found");
        out.println("server: Java HTTP Server: 1.0");
        out.println("Date: " + new Date());
        out.println("Content-Type: text/html");
        out.println();
        out.println("<html>");
        out.println("<body>");
        out.println("<h1>404: Page Not Found</h1>");
        out.println("</body>");
        out.println("</html>");
        out.flush();

        System.out.println("HTTP/1.0 404 Not Found");
        System.out.println("server: Java HTTP Server: 1.0");
        System.out.println("Date: " + new Date());
        System.out.println("Content-Type: text/html");
        System.out.println("");
        System.out.println("<html>");
        System.out.println("<body>");
        System.out.println("<h1>404: Page Not Found</h1>");
        System.out.println("</body>");
        System.out.println("</html>");
    }
    private void sendDirectoryList(PrintWriter out,File directory) throws IOException {
        StringBuilder response = new StringBuilder();
        response.append("HTTP/1.0 200 OK\r\n");
        response.append("Server: Java HTTP Server: 1.0\r\n");
        response.append("Date: " + new Date() + "\r\n");
        response.append("Content-Type: text/html\r\n\r\n");
        response.append("<html><body><h1>Directory listing for " + directory.getName() + "</h1>");
        response.append("<ul>");
        for(File f: Objects.requireNonNull(directory.listFiles())){
            if(f.isDirectory()){
                response.append("<li><b><i><a href=\"" + f.getName() + "/\">" + f.getName() + "/</a></i></b></li>");
            }else {
                response.append("<li><a href=\"" + f.getName() + "\">" + f.getName() + "</a></li>");
            }
        }
        response.append("</ul></body></html>");
        out.println(response.toString());
        out.flush();
        System.out.println(response.toString());
    }
    private void sendFile(String path) throws Exception {
        File file=new File(path);
        FileInputStream fis=new FileInputStream(file);
        String mimeType= URLConnection.guessContentTypeFromName(file.getName());
        if(mimeType==null){
            mimeType="application/octet-stream";
        }
        dataOutputStream.writeBytes("HTTP/1.0 200 OK\r\n");
        dataOutputStream.writeBytes("Server: Java HTTP Server: 1.0\r\n");
        dataOutputStream.writeBytes("Date: " + new Date() + "\r\n");
        System.out.println("HTTP/1.0 200 OK");
        System.out.println("server: Java HTTP Server: 1.0");
        System.out.println("Date: " + new Date());

        //check if other protocol needed
        dataOutputStream.writeBytes("Content-Type: "+mimeType+"\r\n");
        System.out.println(("Content-Type: "+mimeType));
        //sstem.out.println(mimeType);
        if(mimeType.equals("text/plain")||mimeType.equals("image/jpeg")||mimeType.equals("image/png")){
            dataOutputStream.writeBytes("Content-Disposition: inline\r\n");
            System.out.println("Content-Disposition: inline");
        }else {
            dataOutputStream.writeBytes("Content-Disposition: attachment;filename="+file.getName()+"\r\n");
            System.out.println("Content-Disposition: attachment;filename="+file.getName());
        }
        dataOutputStream.writeBytes("Content-Length: "+file.length()+"\r\n");
        System.out.println(("Content-Length: "+file.length()));
        dataOutputStream.writeBytes("\r\n");
        dataOutputStream.flush();
        byte []buffer= new byte[chunk];
        int bytesRead;
        while ((bytesRead=fis.read(buffer))!=-1){
            dataOutputStream.write(buffer,0,bytesRead);
            dataOutputStream.flush();
        }
        fis.close();
    }
    private void recieveFile(String fileName) throws IOException {
        BufferedWriter br=new BufferedWriter(new OutputStreamWriter(connection.getOutputStream()));
        br.write("Confirm");//send confirmation;
        br.newLine();
        br.flush();

        DataInputStream dataInputStream=new DataInputStream(connection.getInputStream());
        //String fileName=dataInputStream.readUTF();
//        String fileName="sqwl";
//        long size=dataInputStream.readLong();
        String filePath=new File("").getAbsolutePath();
        filePath=filePath+("/root/uploaded/"+fileName);
        FileOutputStream fileOutputStream=new FileOutputStream(filePath);
        byte []buffer=new byte[chunk];
        int bytes;
        while((bytes=dataInputStream.read(buffer))!=-1){
            fileOutputStream.write(buffer,0,bytes);
        }
        dataInputStream.close();
        fileOutputStream.close();
        br.close();
        //sstem.out.println("File Successfully uploaded");
    }
}