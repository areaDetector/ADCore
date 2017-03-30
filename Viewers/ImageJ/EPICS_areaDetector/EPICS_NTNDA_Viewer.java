// EPICS_NTNDA_Viewer.java
// Original authors
//      Adapted for EPICS V4 from EPICS_CA_Viewer by Tim Madden
//      Tim Madden, APS
//      Mark Rivers, University of Chicago
import ij.*;
import ij.process.*;
import ij.gui.*;
import java.awt.*;
import ij.plugin.*;
import java.io.*;
import java.text.*;
import java.awt.event.*;
import java.util.*;
import javax.swing.*;
import javax.swing.border.*;

import org.epics.pvaClient.*;
import org.epics.pvaccess.ClientFactory;
import org.epics.pvaccess.client.ChannelProviderRegistry;
import org.epics.pvdata.pv.*;
import org.epics.pvdata.copy.CreateRequest;
import org.epics.pvdata.factory.BasePVUByteArray;
import org.epics.pvdata.factory.ConvertFactory;

import org.epics.nt.*;

public class EPICS_NTNDA_Viewer implements PlugIn
{  ImagePlus img;
  ImageStack imageStack;

  int imageSizeX = 0;
  int imageSizeY = 0;
  int imageSizeZ = 0;
  int colorMode;
  String dataType;

  FileOutputStream debugFile;
  PrintStream debugPrintStream;
  Properties properties = new Properties();
  String propertyFile = "EPICS_NTNDA_Viewer.properties";

  // These are used for the frames/second calculation
  long prevTime;
  int numImageUpdates;

  volatile int UniqueId;

  JFrame frame;

  String PVPrefix;
  JTextField PVPrefixText;
  JTextField NXText;
  JTextField NYText;
  JTextField NZText;
  JTextField FPSText;
  JTextField StatusText;
  JButton startButton;
  JButton stopButton;
  JButton snapButton;

  boolean isDebugMessages;
  boolean isDebugFile;
  boolean isDisplayImages;
  boolean isPluginRunning;
  boolean isSaveToStack;
  boolean isNewStack;
  boolean isConnected;
  volatile boolean isNewImageAvailable;

  javax.swing.Timer timer;

  public static PvaClient pva;
  public PvaClientChannel mychannel;
  public PVStructure read_request;
  public PvaClientMonitor pvamon;
  public PvaClientMonitorData easydata;
  public Convert converter;

  String channame;

  public void run(String arg)
  {
    IJ.showStatus("epics running");
    try
    {
      dataType = new String("none");
      
      
      isDebugFile = false;
      isDebugMessages = false;
      isDisplayImages = false;
      isPluginRunning = true;
      isNewImageAvailable = false;
      isSaveToStack = false;
      isNewStack = false;
      Date date = new Date();
      prevTime = date.getTime();
      numImageUpdates = 0;
      PVPrefix = "13SIM1:Pva1:";
      readProperties();

      if (isDebugFile)
      {
        debugFile = new FileOutputStream(System.getProperty("user.home") +
        System.getProperty("file.separator") + "IJEPICS_debug.txt");
        debugPrintStream = new PrintStream(debugFile);
      }
      javax.swing.SwingUtilities.invokeLater(
      new Runnable()
      {
        public void run()
        {
          createAndShowGUI();
        }
      }
      );
      boolean is_image;
      img = new ImagePlus(PVPrefix, new ByteProcessor(100, 100));
      img.show();
      img.close();
      setupEpics4();

      connectPVs();

      while (isPluginRunning)
      {
        
        if (isDisplayImages)
        {   
          //IJ.log("run:to call waitEvent ");
          
          
          try{
            is_image = pvamon.waitEvent(1);
          }
          catch(Exception ex)
          {
            if (isDebugMessages)
            IJ.log("run: waitEvent throws ");
            is_image=false;
            isDisplayImages=false;
            
          }
          
          //IJ.log("run:done waitEvent ");
          if (is_image)
          {
            
            if (isDebugMessages) IJ.log("calling updateImage");
            
            
            updateImage();
            
            if (isDebugMessages) IJ.log("run:to call releaseEvent ");
            pvamon.releaseEvent();
            if (isDebugMessages) IJ.log("run: called releaseEvent ");
          }// if (is_image)
          
          
        } //if (isDisplayImages)
        else
        {
          Thread.sleep(1000);
        }
        
        

      }
      
      if (isDebugMessages) logMessage("run: Plugin stopping", true, true);
      if (isDebugFile)
      {
        debugPrintStream.close();
        debugFile.close();
        logMessage("Closed debug file", true, true);
      }

      timer.stop();
      writeProperties();
      disconnectPVs();
      
      img.close();

      frame.setVisible(false);

      IJ.showStatus("Exiting Server");

    }
    catch (Exception e)
    {
      IJ.log("run: Got exception: " + e.getMessage());
      e.printStackTrace();
      IJ.log("Close epics CA window, and reopen, try again");

      IJ.showStatus(e.toString());

      try
      {
        if (isDebugFile)
        {
          debugPrintStream.close();
          debugFile.close();
        }
      }
      catch (Exception ee) { }
    }
  }

  public void makeImageCopy()
  {
    ImageProcessor ip = img.getProcessor();
    if (ip == null) return;
    ImagePlus imgcopy = new ImagePlus(PVPrefix + ":" + UniqueId, ip.duplicate());
    imgcopy.show();
  }

  public void setupEpics4()
  {
    logMessage("setupEpics4" , true, true);
    try{
      
      pva=PvaClient.get();
      converter=ConvertFactory.getConvert();
      read_request = CreateRequest.create().createRequest("field()");

    }
    catch (Exception ex)
    {
      logMessage("setupEpics4 Except" , true, true);
      
    }
  }
  
  
  
  public void connectPVs()
  {
    try
    {
      PVPrefix = PVPrefixText.getText();
      logMessage("Trying to connect to EPICS PVs: " + PVPrefix + "Image1", true, true);
      
      
      channame = PVPrefix +"Image";
      mychannel = pva.channel(channame);    
      pvamon=mychannel.createMonitor(read_request);
      pvamon.start();
      easydata = pvamon.getData();        
      checkConnections();
      logMessage("sucessfuylly conn channel/monitor " + channame, true,true);
    }
    catch (Exception ex)
    {
      logMessage("connectPVsExcept:Could not connect to PV: " + channame + ex.getMessage(), true, true);
      
      mychannel = null;
      pvamon = null;
      easydata=null;
      checkConnections();
    }
  }

  public void disconnectPVs()
  {
    try
    {
      pvamon.stop();
      
      logMessage("Called mvamon.stop ", true, true);
    }
    catch (Exception ex)
    {
      logMessage("Cannot disconnect from EPICS PV:" + ex.getMessage(), true, true);
    }
  }

  

  

  public void checkConnections()
  {
    boolean connected;
    isConnected=true;
    
    try
    {
      connected = mychannel.getChannel().isConnected();
      
      if (connected )
      {
        isConnected = true;
        
        PVPrefixText.setBackground(Color.green);
        startButton.setEnabled(!isDisplayImages);
        stopButton.setEnabled(isDisplayImages);
        snapButton.setEnabled(isDisplayImages);
      }
      if (!connected)
      {
        isConnected = false;
        logMessage("Cannot connect to EPICS PVs", true, true);
        PVPrefixText.setBackground(Color.red);
        startButton.setEnabled(false);
        stopButton.setEnabled(false);
        snapButton.setEnabled(false);
      }
    }
    catch (Exception ex)
    {
      // IJ.log("checkConnections: got exception= " + ex.getMessage());
      
      PVPrefixText.setBackground(Color.red);
      startButton.setEnabled(false);
      stopButton.setEnabled(false);
      snapButton.setEnabled(false);
      
      if (isDebugFile) ex.printStackTrace(debugPrintStream);
    } 
  }

  public void updateImage()
  {
    try
    {
      checkConnections();
      if (!isConnected) return;
      
      easydata = pvamon.getData();                 
      
      PVStructure pvs = easydata.getPVStructure();                

      //!! why does wrap return null? it should work?
      NTNDArray myarray =NTNDArray.wrapUnsafe(pvs);
      //The wrap unsafe leaves out most of the NTNDArray fields. but wrap() feils and returns null.
      //    

      int uniqueid = getUniqueId(myarray);
      int ndims = getNumDims(myarray);
      Point oldWindowLocation =null;
      boolean madeNewWindow = false;

      // can bu size, binning, or whatever in dims
      int dimsint[]=getDimsInfo(myarray, "size");

      int nx = dimsint[0];
      int ny = dimsint[1];
      int nz = 1;
      if (ndims>=3)
      nz = dimsint[2];
      
      // int cm = epicsGetInt(ch_colorMode);
      
      int cm  = getAttrValInt( myarray,"ColorMode","value");
      PVScalarArray imagedata = extractImageData( myarray);
      int arraylen = getImageLength(imagedata);
      String dt =  getImageDataType( imagedata);
      
      if (nz == 0) nz = 1;  // 2-D images without color
      if (ny == 0) ny = 1;  // 1-D images which are OK, useful with dynamic profiler
      
      if (isDebugMessages)
      logMessage("UpdateImage: got image, sizes: " + nx + " " + ny + " " + nz,true,true);
      
      int getsize = nx * ny * nz;
      if (getsize == 0) return;  // Not valid dimensions

      if (isDebugMessages)
      logMessage("UpdateImage dt,dataType" + dt+ " "+dataType, true, true);

      if (isDebugMessages)
      logMessage("UpdateImage cm,colorMode" + cm+ " "+colorMode, true, true);

      // if image size changes we must close window and make a new one.
      boolean makeNewWindow = false;
      if (nx != imageSizeX || ny != imageSizeY || nz != imageSizeZ || cm != colorMode || !dt.equals( dataType))
      {
        makeNewWindow = true;
        imageSizeX = nx;
        imageSizeY = ny;
        imageSizeZ = nz;
        colorMode = cm;
        dataType = new String(dt);
        NXText.setText("" + imageSizeX);
        NYText.setText("" + imageSizeY);
        NZText.setText("" + imageSizeZ);
      }

      // If we are making a new stack close the window
      if (isNewStack) makeNewWindow = true;

      // If we need to make a new window then close the current one if it exists
      if (makeNewWindow)
      {
        try
        {
          if (img.getWindow() == null || !img.getWindow().isClosed())
          {
            ImageWindow win = img.getWindow();
            if (win != null) {
              oldWindowLocation = win.getLocationOnScreen();
            }
            img.close();
          }
        }
        catch (Exception ex) { 
          IJ.log("updateImage for exception: " + ex.getMessage());
        }
        makeNewWindow = false;
      }
      // If the window does not exist or is closed make a new one
      if (img.getWindow() == null || img.getWindow().isClosed())
      {
        switch (colorMode)
        {
        case 0:
        case 1:
          if (dataType.equals("byte[]"))
          {
            img = new ImagePlus(PVPrefix, new ByteProcessor(imageSizeX, imageSizeY));
          }
          else if (dataType.equals("short[]") || dataType.equals("ubyte[]") )
          {
            img = new ImagePlus(PVPrefix, new ShortProcessor(imageSizeX, imageSizeY));
          }
          else if (dataType.equals("int[]") || dataType.equals("uint[]") || dataType.equals("float[]") 
              ||  dataType.equals("double[]") || dataType.equals("ushort[]"))
          {
            img = new ImagePlus(PVPrefix, new FloatProcessor(imageSizeX, imageSizeY));
          }
          break;
        case 2:
          img = new ImagePlus(PVPrefix, new ColorProcessor(imageSizeY, imageSizeZ));
          break;
        case 3:
          img = new ImagePlus(PVPrefix, new ColorProcessor(imageSizeX, imageSizeZ));
          break;
        case 4:
          img = new ImagePlus(PVPrefix, new ColorProcessor(imageSizeX, imageSizeY));
          break;
        }
        img.show();
        if (oldWindowLocation != null) img.getWindow().setLocation(oldWindowLocation);
        madeNewWindow = true;
      }

      if (isNewStack)
      {
        imageStack = new ImageStack(img.getWidth(), img.getHeight());
        imageStack.addSlice(PVPrefix + UniqueId, img.getProcessor());
        // Note: we need to add this first slice twice in order to get the slider bar 
        // on the window - ImageJ won't put it there if there is only 1 slice.
        imageStack.addSlice(PVPrefix + UniqueId, img.getProcessor());
        img.close();
        img = new ImagePlus(PVPrefix, imageStack);
        img.show();
        isNewStack = false;
      }

      if (isDebugMessages) IJ.log("about to get pixels");
      if (colorMode == 0 || colorMode == 1)
      {
        if (dataType.equals("byte[]") )
        {            
          byte[] pixels= new byte[arraylen];
          converter.toByteArray(imagedata, 0, arraylen, pixels, 0);

          
          img.getProcessor().setPixels(pixels);
        }
        else if (dataType.equals("short[]") || dataType.equals("ubyte[]")) 
        {
          short[] pixels = new short[arraylen];
          converter.toShortArray(imagedata, 0, arraylen, pixels, 0);
          img.getProcessor().setPixels(pixels);
        }
        else if (dataType.equals("int[]") || dataType.equals("uint[]")|| dataType.equals("float[]")
            || dataType.equals("double[]") ||  dataType.equals("ushort[]"))
        {
          float[] pixels =new float[arraylen];
          converter.toFloatArray(imagedata, 0, arraylen, pixels, 0);
          img.getProcessor().setPixels(pixels);
        }
      }
      else if (colorMode >= 2 && colorMode <= 4)
      {
        int[] pixels = (int[])img.getProcessor().getPixels();
        
        //byte inpixels[] = epicsGetByteArray(ch_image, getsize);
        byte inpixels[]=new byte[getsize];
        
        converter.toByteArray(imagedata, 0, getsize, inpixels, 0);
        switch (colorMode)
        {
        case 2:
          {
            int in = 0, out = 0;
            while (in < getsize)
            {
              pixels[out++] = (inpixels[in++] & 0xFF) << 16 | (inpixels[in++] & 0xFF) << 8 | (inpixels[in++] & 0xFF);
            }
          }
          break;
        case 3:
          {
            int nCols = imageSizeX, nRows = imageSizeZ, row, col;
            int redIn, greenIn, blueIn, out = 0;
            for (row = 0; row < nRows; row++)
            {
              redIn = row * nCols * 3;
              greenIn = redIn + nCols;
              blueIn = greenIn + nCols;
              for (col = 0; col < nCols; col++)
              {
                pixels[out++] = (inpixels[redIn++] & 0xFF) << 16 | (inpixels[greenIn++] & 0xFF) << 8 | (inpixels[blueIn++] & 0xFF);
              }
            }
          }
          break;
        case 4:
          {
            int imageSize = imageSizeX * imageSizeY;
            int redIn = 0, greenIn = imageSize, blueIn = 2 * imageSize, out = 0;
            while (redIn < imageSize)
            {
              pixels[out++] = (inpixels[redIn++] & 0xFF) << 16 | (inpixels[greenIn++] & 0xFF) << 8 | (inpixels[blueIn++] & 0xFF);
            }
          }
          break;
        }
        img.getProcessor().setPixels(pixels);

      }

      if (isSaveToStack)
      {
        img.getStack().addSlice(PVPrefix + UniqueId, img.getProcessor().duplicate());
      }
      img.setSlice(img.getNSlices());
      img.show();
      img.updateAndDraw();
      img.updateStatusbarValue();
      numImageUpdates++;
      // Automatically set brightness and contrast if we made a new window
      if (madeNewWindow) new ContrastEnhancer().stretchHistogram(img, 0.5);
    }
    catch (Exception ex)
    {
      logMessage("UpdateImage got exception: " + ex.getMessage(), true, true);
      if (isDebugFile) ex.printStackTrace(debugPrintStream);
    }
  }

  

  /**
    * Create the GUI and show it.  For thread safety,
    * this method should be invoked from the
    * event-dispatching thread.
    */
  public void createAndShowGUI()
  {
    //Create and set up the window.
    NXText = new JTextField(6);
    NXText.setEditable(false);
    NXText.setHorizontalAlignment(JTextField.CENTER);
    NYText = new JTextField(6);
    NYText.setEditable(false);
    NYText.setHorizontalAlignment(JTextField.CENTER);
    NZText = new JTextField(6);
    NZText.setEditable(false);
    NZText.setHorizontalAlignment(JTextField.CENTER);
    FPSText = new JTextField(6);
    FPSText.setEditable(false);
    FPSText.setHorizontalAlignment(JTextField.CENTER);
    StatusText = new JTextField(40);
    StatusText.setEditable(false);

    PVPrefixText = new JTextField(PVPrefix, 15);
    startButton = new JButton("Start");
    stopButton = new JButton("Stop");
    stopButton.setEnabled(false);
    snapButton = new JButton("Snap");
    JCheckBox captureCheckBox = new JCheckBox("");

    frame = new JFrame("Image J EPICS_NTNDA_Viewer Plugin");
    JPanel panel = new JPanel(new BorderLayout());
    panel.setLayout(new GridBagLayout());
    panel.setBorder(new EmptyBorder(new Insets(5, 5, 5, 5)));
    frame.getContentPane().add(BorderLayout.CENTER, panel);
    GridBagConstraints c = new GridBagConstraints();
    // Add extra space around each component to avoid clutter
    c.insets = new Insets(2, 2, 2, 2);

    // Top row
    // Anchor all components CENTER
    c.anchor = GridBagConstraints.CENTER;
    c.gridx = 0;
    c.gridy = 0;
    panel.add(new JLabel("PVPrefix"), c);
    c.gridx = 1;
    panel.add(new JLabel("NX"), c);
    c.gridx = 2;
    panel.add(new JLabel("NY"), c);
    c.gridx = 3;
    panel.add(new JLabel("NZ"), c);
    c.gridx = 4;
    panel.add(new JLabel("Frames/s"), c);
    c.gridx = 5;
    panel.add(new JLabel("Capture to Stack"), c);

    // Middle row
    // These widgets should be centered
    c.anchor = GridBagConstraints.CENTER;
    c.gridy = 1;
    c.gridx = 0;
    panel.add(PVPrefixText, c);
    c.gridx = 1;
    panel.add(NXText, c);
    c.gridx = 2;
    panel.add(NYText, c);
    c.gridx = 3;
    panel.add(NZText, c);
    c.gridx = 4;
    panel.add(FPSText, c);
    c.gridx = 5;
    panel.add(captureCheckBox, c);
    c.gridx = 6;
    panel.add(snapButton, c);
    c.gridx = 7;
    panel.add(startButton, c);
    c.gridx = 8;
    panel.add(stopButton, c);

    // Bottom row
    c.gridy = 2;
    c.gridx = 0;
    c.anchor = GridBagConstraints.EAST;
    panel.add(new JLabel("Status: "), c);
    c.gridx = 1;
    c.gridwidth = 7;
    c.anchor = GridBagConstraints.WEST;
    panel.add(StatusText, c);

    //Display the window.
    frame.pack();
    frame.addWindowListener(new FrameExitListener());
    frame.setVisible(true);

    int timerDelay = 2000;  // 2 seconds 
    timer = new javax.swing.Timer(timerDelay, new ActionListener()
    {
      public void actionPerformed(ActionEvent event)
      {
        checkConnections();
        long time = new Date().getTime();
        double fps = 1000. * numImageUpdates / (double)(time - prevTime);
        NumberFormat form = DecimalFormat.getInstance();
        ((DecimalFormat)form).applyPattern("0.0");
        FPSText.setText("" + form.format(fps));
        if (isDisplayImages && numImageUpdates > 0) logMessage("New images=" + numImageUpdates, true, false);
        prevTime = time;
        numImageUpdates = 0;
      }
    });
    timer.start();

    PVPrefixText.addActionListener(new ActionListener()
    {
      public void actionPerformed(ActionEvent event)
      {
        disconnectPVs();
        connectPVs();
        checkConnections();
      }
    });

    startButton.addActionListener(new ActionListener()
    {
      public void actionPerformed(ActionEvent event)
      {
        startButton.setEnabled(false);
        stopButton.setEnabled(true);
        snapButton.setEnabled(true);
        isDisplayImages = true;
        logMessage("Image display started", true, true);
      }
    });

    stopButton.addActionListener(new ActionListener()
    {
      public void actionPerformed(ActionEvent event)
      {
        startButton.setEnabled(true);
        stopButton.setEnabled(false);
        snapButton.setEnabled(false);
        isDisplayImages = false;
        logMessage("Image display stopped", true, true);
      }
    });

    snapButton.addActionListener(new ActionListener()
    {
      public void actionPerformed(ActionEvent event)
      {
        makeImageCopy();
      }
    });

    captureCheckBox.addItemListener(new ItemListener()
    {
      public void itemStateChanged(ItemEvent e)
      {
        if (e.getStateChange() == ItemEvent.SELECTED)
        {
          isSaveToStack = true;
          isNewStack = true;
          IJ.log("record on");
        }
        else
        {
          isSaveToStack = false;
          IJ.log("record off");
        }

      }
    }
    );

  }

  public class FrameExitListener extends WindowAdapter
  {
    public void windowClosing(WindowEvent event)
    {
      isPluginRunning = false;
      isNewImageAvailable = false;
      // We need to wake up the main thread so it shuts down cleanly
      synchronized (this)
      {
        notify();
      }
    }
  }

  public void logMessage(String message, boolean logDisplay, boolean logFile)
  {
    Date date = new Date();
    SimpleDateFormat simpleDate = new SimpleDateFormat("d/M/y k:m:s.S");
    String completeMessage;

    completeMessage = simpleDate.format(date) + ": " + message;
    if (logDisplay) StatusText.setText(completeMessage);
    if (logFile) IJ.log(completeMessage);
  }

  public void readProperties()
  {
    String temp, path = null;

    try
    {
      String fileSep = System.getProperty("file.separator");
      path = System.getProperty("user.home") + fileSep + propertyFile;
      FileInputStream file = new FileInputStream(path);
      properties.load(file);
      file.close();
      temp = properties.getProperty("PVPrefix");
      if (temp != null) PVPrefix = temp;
      IJ.log("Read properties file: " + path + "  PVPrefix= " + PVPrefix);
    }
    catch (Exception ex)
    {
      IJ.log("readProperties:exception: " + ex.getMessage());
    }
  }

  public void writeProperties()
  {
    String path;
    try
    {
      String fileSep = System.getProperty("file.separator");
      path = System.getProperty("user.home") + fileSep + propertyFile;
      properties.setProperty("PVPrefix", PVPrefix);
      FileOutputStream file = new FileOutputStream(path);
      properties.store(file, "EPICS_AD_Viewer Properties");
      file.close();
      IJ.log("Wrote properties file: " + path);
    }
    catch (Exception ex)
    {
      IJ.log("writeProperties:exception: " + ex.getMessage());
    }
  }
  
  /**
    * return num of dimensions in the NDArray
    * @param myarray
    * @return
    */
  public int getNumDims(NTNDArray myarray)
  {
    int ndims=myarray.getDimension().getLength();        
    return(ndims);
  }
  
  
  public void printMonDataStruct()
  {

    PVStructure pvs = easydata.getPVStructure();
    String [] fnames = easydata.getStructure().getFieldNames();
    
    for (int m=0;m<fnames.length;m++)
    System.out.println(fnames[m]);
    
  }
  
  public int getUniqueId(NTNDArray myarray)
  {
    int uniqueid  =myarray.getUniqueId().get();
    return(uniqueid);
  }
  
  public String getDimsString(NTNDArray myarray)
  {
    PVStructureArray pvdim = myarray.getDimension();
    String dimstring =pvdim.toString();
    return(dimstring);

  }
  
  
  public int[] getDimsInfo(NTNDArray myarray,String whichinfo)
  {        
    int ndims = this.getNumDims(myarray);
    int dimsint[] = new int[ndims];
    PVStructureArray pvdim = myarray.getDimension();

    StructureArrayData dimdata=new StructureArrayData();
    pvdim.get(0,ndims,dimdata);
    
    for (int kk = 0;kk<ndims;kk++)
    {
      PVField[] dimfields = dimdata.data[kk].getPVFields();
      for (int km = 0;km<dimfields.length;km++)
      {
        String dfname = dimfields[km].getField().getID();
        String dfn2=dimfields[km].getFieldName();
        //System.out.println(dfname + " "+dfn2);
        if (dfn2.equals(whichinfo))
        {
          
          dimsint[kk]=converter.toInt((PVScalar)dimfields[km]);
        }
      }
    }
    
    //int nf = pvdim.getNumberFields();
    
    //String dimstring =pvdim.toString();
    return(dimsint);
  }

  
  String getImageDataType(PVScalarArray imagedata)
  {

    //pure; intruspection.
    Field pvuff=imagedata.getField();
    
    //tells its a scalar array
    Type pvufft = pvuff.getType();
    //returns like "scalarArray"
    String pvuffts = pvufft.toString();
    //returns something like 'ushort[]'
    String arraytype  =pvuff.getID();
    return(arraytype);

  }
  
  
  public int getImageLength(PVScalarArray imgdata)
  {
    
    // this code works, but we shorten below...This is all introspective
    //PVField has both data and introspection. 
    int arraylen  =imgdata.getLength();
    
    return(arraylen);
    
  }
  
  PVScalarArray extractImageData(NTNDArray myarray)
  {
    //So I can get the data. How do I know the Union is holdibng bytes[], floats[] or ints[]?
    // Not sure how to ask NTNDArray the data type
    PVUnion pvu = myarray.getValue();
    // this code works, but we shorten below...This is all introspective
    //PVField has both data and introspection. 
    PVField pvuf = pvu.get();
    return((PVScalarArray)pvuf);
  }
  
  
  public int getNumAttributes(NTNDArray myarray)
  {
    int nattribs=myarray.getAttribute().getLength();
    return(nattribs);
    
  }

  public String getAttrType(NTNDArray myarray,String attrname,String attrfield) 
  {
    int nattr = this.getNumAttributes(myarray);
    String attrval=new String("unknown");
    PVStructureArray attr1 = myarray.getAttribute();
    StructureArrayData attr2=new StructureArrayData();
    attr1.get(0,nattr,attr2);
    for (int kk = 0;kk<nattr;kk++)
    {            
      PVField[] attrfields = attr2.data[kk].getPVFields();
      for (int km = 0;km<attrfields.length;km++)
      {
        String dfn2=attrfields[km].getFieldName();
        if (dfn2.equals("name"))
        {                    
          String aname = converter.toString(((PVString)attrfields[km]));                    
          if (aname.equals(attrname))
          {
            for (int mm = 0;mm<attrfields.length;mm++)
            {
              String dfn3=attrfields[mm].getFieldName();
              if (dfn3.equals(attrfield))
              {
                String t =  attrfields[mm].getField().getType().toString();
                
                if (t.equals("union"))
                {
                  PVUnion apvu = (PVUnion)attrfields[mm];
                  PVField apvuf = apvu.get();
                  String s1 = apvuf.getField().getID();                                    
                  return(s1);                                
                }

              }                                
            }
          }
          
        }                                
      }
    }        
    return(attrval);
  }

  public int getAttrValInt(NTNDArray myarray,String attrname,String attrfield) 
  {
    int nattr = this.getNumAttributes(myarray);
    int attrval=0;
    PVStructureArray attr1 = myarray.getAttribute();
    StructureArrayData attr2=new StructureArrayData();
    attr1.get(0,nattr,attr2);
    for (int kk = 0;kk<nattr;kk++)
    {            
      PVField[] attrfields = attr2.data[kk].getPVFields();
      for (int km = 0;km<attrfields.length;km++)
      {
        String dfn2=attrfields[km].getFieldName();
        if (dfn2.equals("name"))
        {                    
          String aname = converter.toString(((PVString)attrfields[km]));                    
          if (aname.equals(attrname))
          {
            for (int mm = 0;mm<attrfields.length;mm++)
            {
              String dfn3=attrfields[mm].getFieldName();
              if (dfn3.equals(attrfield))
              {
                String t =  attrfields[mm].getField().getType().toString();                                
                if (t.equals("union"))
                {
                  PVUnion apvu = (PVUnion)attrfields[mm];
                  PVField apvuf = apvu.get();
                  String s1 = apvuf.getField().getID();
                  
                  if (s1.equals("int"))
                  {
                    PVInt atri=(PVInt)apvuf;
                    attrval = atri.get();
                  }
                  else
                  System.out.println("Error- Wrong attr type");
                }
                return(attrval);
              }                                
            }
          }                    
        }                                
      }
    }
    return(attrval);
  }

  
  public double getAttrValDouble(NTNDArray myarray,String attrname,String attrfield) 
  {        
    int nattr = this.getNumAttributes(myarray);
    double attrval=0.0;        
    PVStructureArray attr1 = myarray.getAttribute();
    StructureArrayData attr2=new StructureArrayData();
    attr1.get(0,nattr,attr2);
    for (int kk = 0;kk<nattr;kk++)
    {            
      PVField[] attrfields = attr2.data[kk].getPVFields();
      for (int km = 0;km<attrfields.length;km++)
      {
        String dfn2=attrfields[km].getFieldName();
        if (dfn2.equals("name"))
        {                    
          String aname = converter.toString(((PVString)attrfields[km]));                    
          if (aname.equals(attrname))
          {
            for (int mm = 0;mm<attrfields.length;mm++)
            {
              String dfn3=attrfields[mm].getFieldName();
              if (dfn3.equals(attrfield))
              {
                String t =  attrfields[mm].getField().getType().toString();                                
                if (t.equals("union"))
                {
                  PVUnion apvu = (PVUnion)attrfields[mm];
                  PVField apvuf = apvu.get();
                  String s1 = apvuf.getField().getID();
                  
                  if (s1.equals("double"))
                  {
                    PVDouble atri=(PVDouble)apvuf;
                    attrval = atri.get();
                  }
                  else
                  System.out.println("Error- Wrong attr type");
                  
                }                                
                return(attrval);
              }                                
            }
          }                   
        }                                
      }
    }
    return(attrval);
  }

  
  

}

