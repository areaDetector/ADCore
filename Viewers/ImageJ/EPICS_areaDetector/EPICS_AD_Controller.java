// EPICS_AD_Viewer.java
// Original authors
//      Tim Madden, APS
//      Mark Rivers, University of Chicago

import ij.*;
import ij.process.*;
import java.awt.*;
import ij.plugin.*;
import ij.gui.ImageCanvas;
import ij.gui.ImageWindow;
import java.io.*;
import java.text.*;
import java.awt.event.*;
import java.util.*;
import javax.swing.*;
import javax.swing.border.*;

import gov.aps.jca.*;
import gov.aps.jca.dbr.*;
import gov.aps.jca.configuration.*;
import gov.aps.jca.event.*;

public class EPICS_AD_Viewer implements PlugIn, MouseListener {
    
    private static final int MAX_TRANSFORMS = 4;

    private static final short LOWER_LEFT = 0;
    private static final short UPPER_LEFT = 1;
    private static final short LOWER_RIGHT = 2;
    private static final short UPPER_RIGHT = 3;

    private static final short NONE = 0;
    private static final short ROTATE_90_CW = 1;
    private static final short ROTATE_90_CCW = 2;
    private static final short ROTATE_180 = 3;
    private static final short FLIP_0011 = 4;
    private static final short FLIP_0110 = 5;
    private static final short FLIP_X = 6;
    private static final short FLIP_Y = 7;

    ImagePlus img;
    ImageStack imageStack;

    int imageSizeX = 0;
    int imageSizeY = 0;
    int imageSizeZ = 0;
    int colorMode;
    DBRType dataType;

    FileOutputStream debugFile;
    PrintStream debugPrintStream;
    Properties properties = new Properties();
    String propertyFile = "EPICS_AD_Viewer.properties";

    // These are used for the frames/second calculation
    long prevTime;
    int numImageUpdates;

    JCALibrary jca;
    DefaultConfiguration conf;
    Context ctxt;

    /**
     * these are EPICS channel objects to get images...
     */
    Channel ch_nx;
    Channel ch_ny;
    Channel ch_nz;
    Channel ch_colorMode;
    Channel ch_image;
    Channel ch_image_id;
    volatile int UniqueId;

    /* These are EPICS channel objects to write the ROI
     * back to the areaDetector software.
     */
    Channel ch_minCamX;           //This is the start of the CCD readout in X
    Channel ch_minCamY;           //This is the start of the CCD readout in Y
    Channel ch_minCamX_RBV;       //This is the readback value of X of the CCD readout
    Channel ch_minCamY_RBV;       //This is the readback value of Y of the CCD readout
    Channel ch_sizeCamX;          //This is the size of the CCD readout in the X dimension
    Channel ch_sizeCamY;          //This is the size of the CCD readout in the Y dimension
    Channel ch_sizeCamArrayX_RBV; //This is the size of the image array of the CCD in X
    Channel ch_sizeCamArrayY_RBV; //This is the size of the image array of the CCD in Y
    Channel ch_camBinX_RBV;       //This is the amount of binning for the CCD readout in X
    Channel ch_camBinY_RBV;       //This is the amount of binning for the CCD readout in Y
    Channel ch_minRoiX;           //This is the start of the ROI in X
    Channel ch_minRoiY;           //This is the start of the ROI in Y
    Channel ch_minRoiX_RBV;       //This is the readback value of X in the ROI
    Channel ch_minRoiY_RBV;       //This is the readback value of Y in the ROI
    Channel ch_sizeRoiX;          //This is the size of the ROI in the X dimension
    Channel ch_sizeRoiY;          //This is the size of the ROI in the Y dimension
    Channel ch_sizeRoiArrayX_RBV; //This is the size of the image array for the ROI in X
    Channel ch_sizeRoiArrayY_RBV; //This is the size of the image array for the ROI in Y
    Channel ch_roiBinX_RBV;       //This is the amount of binning for the ROI in X
    Channel ch_roiBinY_RBV;       //This is the amount of binning for the ROI in Y
    Channel ch_origin;            //This represents the location of the origin
    Channel ch_maxSizeCamX;       //This is the maximum dimension of X
    Channel ch_maxSizeCamY;       //This is the maximum dimension of Y
    Channel[] ch_transType = new Channel[MAX_TRANSFORMS];//This represents the way the image is flipped or rotated.

    JFrame frame;

    String imagePrefix;
    JTextField imagePrefixText;
    JTextField NXText;
    JTextField NYText;
    JTextField NZText;
    JTextField FPSText;
    JTextField StatusText;
    JTextField cameraPrefixText;
    JTextField transformPrefixText;
    JTextField roiPrefixText;
    JButton startButton;
    JButton stopButton;
    JButton snapButton;
    JButton resetRoiButton;
    JRadioButton setNoneRadioButton;
    JRadioButton setCCDReadoutRadioButton;
    JRadioButton setROIRadioButton;
    JRadioButton preTransformRadioButton;
    JRadioButton postTransformRadioButton;
    JCheckBox useTransformCheckBox;

    boolean isDebugMessages;
    boolean isDebugFile;
    boolean isDisplayImages;
    boolean isPluginRunning;
    boolean isSaveToStack;
    boolean isNewStack;
    boolean isImageConnected;
    boolean isTransformConnected;
    boolean isCameraConnected;
    boolean isRoiConnected;
    boolean setCCD_Readout;
    boolean setROI;
    boolean useTransformPlugin;
    boolean isPreTransform;
    boolean isPostTransform;
    volatile boolean isNewImageAvailable;

    javax.swing.Timer timer;

    int cameraOriginX;
    int cameraOriginY;
    int cameraArraySizeX;
    int cameraArraySizeY;
    int cameraBinX;
    int cameraBinY;
    int roiOriginX;
    int roiOriginY;
    int roiArraySizeX;
    int roiArraySizeY;
    int roiBinX;
    int roiBinY;
    String cameraPrefix;
    String transformPrefix;
    String roiPrefix;
    ImageWindow win;
    ImageCanvas canvas;

    /**
     * This method is called by ImageJ when the user starts the from the menu.
     * 
     * @param arg    This is a string argument that can be used to pass in
     *               information.
     */
    @Override
    public void run(String arg) {
        IJ.showStatus("epics running");
        try {
            isDebugFile = false;
            isDebugMessages = false;
            isDisplayImages = false;
            isPluginRunning = true;
            isNewImageAvailable = false;
            isSaveToStack = false;
            isNewStack = false;
            setCCD_Readout = false;
            setROI = false;
            useTransformPlugin = false;
            isPreTransform = false;
            isPostTransform = false;
            Date date = new Date();
            prevTime = date.getTime();
            numImageUpdates = 0;
            imagePrefix = "13SIM1:image1:";
            cameraPrefix = "13SIM1:cam1:";
            transformPrefix = "13SIM1:Trans1:";
            roiPrefix = "13SIM1:ROI1:";
            readProperties();

            if (isDebugFile) {
                debugFile = new FileOutputStream(System.getProperty("user.home")
                        + System.getProperty("file.separator") + "IJEPICS_debug.txt");
                debugPrintStream = new PrintStream(debugFile);
            }
            javax.swing.SwingUtilities.invokeLater(
                    new Runnable() {
                        @Override
                        public void run() {
                            createAndShowGUI();
                        }
                    }
            );

            img = new ImagePlus(imagePrefix, new ByteProcessor(100, 100));
            img.show();
            img.close();

            startEPICSCA();
            connectImagePVs();
            connectCameraPVs();
            connectTransformPVs();
            connectRoiPVs();

            /* This simply polls for new data and updates the image if new data
             * is found.
             */
            while (isPluginRunning) {
                synchronized (this) {
                    wait(1000);
                }
                if (isDisplayImages && isNewImageAvailable) {
                    if (isDebugMessages) {
                        IJ.log("calling updateImage");
                    }
                    updateImage();
                    isNewImageAvailable = false;
                }
            }

            if (isDebugFile) {
                debugPrintStream.close();
                debugFile.close();
                logMessage("Closed debug file", true, true);
            }

            timer.stop();
            writeProperties();
            disconnectImagePVs();
            disconnectCameraPVs();
            disconnectTransformPVs();
            disconnectRoiPVs();
            closeEPICSCA();
            img.close();

            frame.setVisible(false);

            IJ.showStatus("Exiting Server");

        } catch (Exception e) {
            IJ.log("Got exception: " + e.getMessage());
            e.printStackTrace();
            IJ.log("Close epics CA window, and reopen, try again");

            IJ.showStatus(e.toString());

            try {
                if (isDebugFile) {
                    debugPrintStream.close();
                    debugFile.close();
                }
            } catch (IOException ee) {
            }
        }
    }
    
    /**
     * This method overrides the standard mouseReleased method for the image
     * window.  When the use releases the mouse, it gets the rectangle drawn
     * by the user and uses that to write an ROI to either the camera or the
     * ROI plugin.  If no rectangle is drawn, the values retrieved are 0,0 and
     * the full dimensions in X and Y.
     * 
     * @param e     An event object.  It is not actually used.
     */
    @Override
    public void mouseReleased(MouseEvent e) {
        Rectangle imageRoi;

        int tempRoiX;
        int tempRoiY;
        int tempRoiWidth;
        int tempRoiHeight;
        int tempMaxSizeX;
        int tempMaxSizeY;

        if ((setCCD_Readout || (setROI && isRoiConnected)) && isCameraConnected) {
            try {
                imageRoi = this.img.getProcessor().getRoi();
                tempRoiX = (int) imageRoi.getX();
                tempRoiY = (int) imageRoi.getY();
                tempRoiWidth = (int) imageRoi.getWidth();
                tempRoiHeight = (int) imageRoi.getHeight();

                if (setROI) {
                    tempMaxSizeX = roiArraySizeX;
                    tempMaxSizeY = roiArraySizeY;
                }
                else {
                    tempMaxSizeX = cameraArraySizeX;
                    tempMaxSizeY = cameraArraySizeY;
                }
                
                if(isDebugMessages)
                    IJ.log("ROI values: X=" + tempRoiX + ", Y=" + tempRoiY + ", W="
                                + tempRoiWidth + ", H=" + tempRoiHeight + ", MaxX="
                                + tempMaxSizeX + ", MaxY=" + tempMaxSizeY);

                calculateROI(tempRoiX, tempRoiY, tempRoiWidth, tempRoiHeight, tempMaxSizeX, tempMaxSizeY);
                
            } catch (Exception ex) {
                logMessage("Cannot set ROI:" + ex.getMessage(), true, true);
            }
        }
    }

    @Override
    public void mouseExited(MouseEvent e) {
    }

    @Override
    public void mouseEntered(MouseEvent e) {
    }

    @Override
    public void mousePressed(MouseEvent e) {
    }

    @Override
    public void mouseClicked(MouseEvent e) {
    }

    public void makeImageCopy() {
        ImageProcessor dipcopy = img.getProcessor().duplicate();
        ImagePlus imgcopy = new ImagePlus(imagePrefix + ":" + UniqueId, dipcopy);
        imgcopy.show();
    }

    /**
     * This method creates the PV objects for the image plugin.
     */
    public void connectImagePVs() {
        try {
            imagePrefix = imagePrefixText.getText();
            logMessage("Trying to connect to EPICS PVs: " + imagePrefix, true, true);
            if (isDebugFile) {
                debugPrintStream.println("Trying to connect to EPICS PVs: " + imagePrefix);
                debugPrintStream.println("context.printfInfo  ****************************");
                debugPrintStream.println();
                ctxt.printInfo(debugPrintStream);

                debugPrintStream.print("jca.printInfo  ****************************");
                debugPrintStream.println();
                jca.printInfo(debugPrintStream);
                debugPrintStream.print("jca.listProperties  ****************************");
                debugPrintStream.println();
                jca.listProperties(debugPrintStream);
            }
            ch_nx = createEPICSChannel(imagePrefix + "ArraySize0_RBV");
            ch_ny = createEPICSChannel(imagePrefix + "ArraySize1_RBV");
            ch_nz = createEPICSChannel(imagePrefix + "ArraySize2_RBV");
            ch_colorMode = createEPICSChannel(imagePrefix + "ColorMode_RBV");
            ch_image = createEPICSChannel(imagePrefix + "ArrayData");
            ch_image_id = createEPICSChannel(imagePrefix + "UniqueId_RBV");
            ch_image_id.addMonitor(
                    Monitor.VALUE,
                    new newUniqueIdCallback()
            );
            
            ctxt.flushIO();
            checkImagePVConnections();
        } catch (Exception ex) {
            logMessage("CAException: Cannot connect to EPICS image PV:" + ex.getMessage(), true, true);
            checkImagePVConnections();
        }
    }
    
    /**
     * This method creates the PV objects for the camera.
     */
    public void connectCameraPVs() {
        try {
            cameraPrefix = cameraPrefixText.getText();
            logMessage("Trying to connect to EPICS PVs: " + cameraPrefix, true, true);
            if (isDebugFile) {
                debugPrintStream.println("Trying to connect to EPICS PVs: " + cameraPrefix);
                debugPrintStream.println("context.printfInfo  ****************************");
                debugPrintStream.println();
                ctxt.printInfo(debugPrintStream);

                debugPrintStream.print("jca.printInfo  ****************************");
                debugPrintStream.println();
                jca.printInfo(debugPrintStream);
                debugPrintStream.print("jca.listProperties  ****************************");
                debugPrintStream.println();
                jca.listProperties(debugPrintStream);
            }
            
            ch_minCamX = createEPICSChannel(cameraPrefix + "MinX");
            ch_minCamY = createEPICSChannel(cameraPrefix + "MinY");
            ch_minCamX_RBV = createEPICSChannel(cameraPrefix + "MinX_RBV");
            ch_minCamY_RBV = createEPICSChannel(cameraPrefix + "MinY_RBV");
            ch_sizeCamX = createEPICSChannel(cameraPrefix + "SizeX");
            ch_sizeCamY = createEPICSChannel(cameraPrefix + "SizeY");
            ch_sizeCamArrayX_RBV = createEPICSChannel(cameraPrefix + "ArraySizeX_RBV");
            ch_sizeCamArrayY_RBV = createEPICSChannel(cameraPrefix + "ArraySizeY_RBV");
            ch_maxSizeCamX = createEPICSChannel(cameraPrefix + "MaxSizeX_RBV");
            ch_maxSizeCamY = createEPICSChannel(cameraPrefix + "MaxSizeY_RBV");
            ch_camBinX_RBV = createEPICSChannel(cameraPrefix + "BinX_RBV");
            ch_camBinY_RBV = createEPICSChannel(cameraPrefix + "BinY_RBV");
            ch_sizeCamArrayX_RBV.addMonitor(Monitor.VALUE, new newCameraArraySizeX());
            ch_sizeCamArrayY_RBV.addMonitor(Monitor.VALUE, new newCameraArraySizeY());
            
            ctxt.flushIO();
            checkCameraPVConnections();
            
        } catch (Exception ex) {
            logMessage("CAException: Cannot connect to EPICS camera PV:" + ex.getMessage(), true, true);
            checkCameraPVConnections();
        }
    }
    
    /**
     * This method creates the PV objects for the transform plugin.
     */
    public void connectTransformPVs() {
        try {
            transformPrefix = transformPrefixText.getText();
            logMessage("Trying to connect to EPICS PVs: " + transformPrefix, true, true);
            if (isDebugFile) {
                debugPrintStream.println("Trying to connect to EPICS PVs: " + transformPrefix);
                debugPrintStream.println("context.printfInfo  ****************************");
                debugPrintStream.println();
                ctxt.printInfo(debugPrintStream);

                debugPrintStream.print("jca.printInfo  ****************************");
                debugPrintStream.println();
                jca.printInfo(debugPrintStream);
                debugPrintStream.print("jca.listProperties  ****************************");
                debugPrintStream.println();
                jca.listProperties(debugPrintStream);
            }
            
            ch_transType[0] = createEPICSChannel(transformPrefix + "Type1");
            ch_transType[1] = createEPICSChannel(transformPrefix + "Type2");
            ch_transType[2] = createEPICSChannel(transformPrefix + "Type3");
            ch_transType[3] = createEPICSChannel(transformPrefix + "Type4");
            ch_origin = createEPICSChannel(transformPrefix + "OriginLocation");

            ctxt.flushIO();
            checkTransformPVConnections();
            
        } catch (Exception ex) {
            logMessage("CAException: Cannot connect to EPICS transform PV:" + ex.getMessage(), true, true);
            checkTransformPVConnections();
        }
    }
    
    /**
     * This method creates the PV objects for the ROI plugin.
     */
    public void connectRoiPVs() {
        try {
            roiPrefix = roiPrefixText.getText();
            logMessage("Trying to connect to EPICS PVs: " + roiPrefix, true, true);
            if (isDebugFile) {
                debugPrintStream.println("Trying to connect to EPICS PVs: " + roiPrefix);
                debugPrintStream.println("context.printfInfo  ****************************");
                debugPrintStream.println();
                ctxt.printInfo(debugPrintStream);

                debugPrintStream.print("jca.printInfo  ****************************");
                debugPrintStream.println();
                jca.printInfo(debugPrintStream);
                debugPrintStream.print("jca.listProperties  ****************************");
                debugPrintStream.println();
                jca.listProperties(debugPrintStream);
            }
            
            ch_minRoiX = createEPICSChannel(roiPrefix + "MinX");
            ch_minRoiY = createEPICSChannel(roiPrefix + "MinY");
            ch_minRoiX_RBV = createEPICSChannel(roiPrefix + "MinX_RBV");
            ch_minRoiY_RBV = createEPICSChannel(roiPrefix + "MinY_RBV");
            ch_sizeRoiX = createEPICSChannel(roiPrefix + "SizeX");
            ch_sizeRoiY = createEPICSChannel(roiPrefix + "SizeY");
            ch_roiBinX_RBV = createEPICSChannel(roiPrefix + "BinX_RBV");
            ch_roiBinY_RBV = createEPICSChannel(roiPrefix + "BinY_RBV");
            ch_sizeRoiArrayX_RBV = createEPICSChannel(roiPrefix + "ArraySizeX_RBV");
            ch_sizeRoiArrayY_RBV = createEPICSChannel(roiPrefix + "ArraySizeY_RBV");
            ch_sizeRoiArrayX_RBV.addMonitor(Monitor.VALUE, new newRoiArraySizeX());
            ch_sizeRoiArrayY_RBV.addMonitor(Monitor.VALUE, new newRoiArraySizeY());
            
            ctxt.flushIO();
            checkRoiPVConnections();
            
        } catch (Exception ex) {
            logMessage("CAException: Cannot connect to EPICS ROI PV:" + ex.getMessage(), true, true);
            checkRoiPVConnections();
        }  
    }

    /**
     * This method destroys the PV objects for the image plugin.
     */
    public void disconnectImagePVs() {
        try {
            ch_nx.destroy();
            ch_ny.destroy();
            ch_nz.destroy();
            ch_colorMode.destroy();
            ch_image.destroy();
            ch_image_id.destroy();
            isImageConnected = false;
            
            logMessage("Disconnected from EPICS image PVs OK", true, true);
        } catch (CAException ex) {
            logMessage("CAException: Cannot disconnect from EPICS image PV:" + ex.getMessage(), true, true);
        } catch (IllegalStateException ex) {
            logMessage("IllegalStateException: Cannot disconnect from EPICS image PV:" + ex.getMessage(), true, true);
        } catch (NullPointerException ex) {
            logMessage("NullPointerException: Cannot disconnect from EPICS image PV:" + ex.getMessage(), true, true);
        }
    }
    
    /**
     * This method destroys the PV objects for the camera.
     */
    public void disconnectCameraPVs() {
        try {
            ch_minCamX.destroy();
            ch_minCamY.destroy();
            ch_minCamX_RBV.destroy();
            ch_minCamY_RBV.destroy();
            ch_sizeCamX.destroy();
            ch_sizeCamY.destroy();
            ch_sizeCamArrayX_RBV.destroy();
            ch_sizeCamArrayY_RBV.destroy();
            ch_maxSizeCamX.destroy();
            ch_maxSizeCamY.destroy();
            ch_camBinX_RBV.destroy();
            ch_camBinY_RBV.destroy();
            isCameraConnected = false;
            
            logMessage("Disconnected from EPICS camera PVs OK", true, true);
        }catch (CAException ex) {
            logMessage("CAException: Cannot disconnect from EPICS camera PV:" + ex.getMessage(), true, true);
        } catch (IllegalStateException ex) {
            logMessage("IllegalStateException: Cannot disconnect from EPICS camera PV:" + ex.getMessage(), true, true);
        } catch (NullPointerException ex) {
            logMessage("NullPointerException: Cannot disconnect from EPICS camera PV:" + ex.getMessage(), true, true);
        }
    }
    
    /**
     * This method destroys the PV objects for the transform plugin.
     */
    public void disconnectTransformPVs() {
        try {
            ch_transType[0].destroy();
            ch_transType[1].destroy();
            ch_transType[2].destroy();
            ch_transType[3].destroy();
            ch_origin.destroy();
            isTransformConnected = false;
            
            logMessage("Disconnected from EPICS transform PVs OK", true, true);
        } catch (CAException ex) {
            logMessage("CAException: Cannot disconnect from EPICS transform PV:" + ex.getMessage(), true, true);
        } catch (IllegalStateException ex) {
            logMessage("IllegalStateException: Cannot disconnect from EPICS tansform PV:" + ex.getMessage(), true, true);
        } catch (NullPointerException ex) {
            logMessage("NullPointerException: Cannot disconnect from EPICS transform PV:" + ex.getMessage(), true, true);
        }
    }
    
    /**
     * This method destroys the PV objects for the ROI plugin.
     */
    public void disconnectRoiPVs() {
        try {
            ch_minRoiX.destroy();
            ch_minRoiY.destroy();
            ch_minRoiX_RBV.destroy();
            ch_minRoiY_RBV.destroy();
            ch_sizeRoiX.destroy();
            ch_sizeRoiY.destroy();
            ch_sizeRoiArrayX_RBV.destroy();
            ch_sizeRoiArrayY_RBV.destroy();
            ch_roiBinX_RBV.destroy();
            ch_roiBinY_RBV.destroy();
            isRoiConnected = false;
            
            logMessage("Disconnected from EPICS ROI PVs OK", true, true);
        } catch (CAException ex) {
            logMessage("CAException: Cannot disconnect from EPICS ROI PV:" + ex.getMessage(), true, true);
        } catch (IllegalStateException ex) {
            logMessage("IllegalStateException: Cannot disconnect from EPICS ROI PV:" + ex.getMessage(), true, true);
        } catch (NullPointerException ex) {
            logMessage("NullPointerException: Cannot disconnect from EPICS ROI PV:" + ex.getMessage(), true, true);
        }
    }

    /**
     * This method creates the channel access context and initializes it.
     */
    public void startEPICSCA() {
        logMessage("Initializing EPICS", true, true);

        try {
            System.setProperty("jca.use_env", "true");
            // Get the JCALibrary instance.
            jca = JCALibrary.getInstance();
            ctxt = jca.createContext(JCALibrary.CHANNEL_ACCESS_JAVA);
            ctxt.initialize();
        } catch (CAException ex) {
            logMessage("startEPICSCA exception: " + ex.getMessage(), true, true);
        }

    }

    /**
     * This method destroys the channel access context.
     * 
     * @throws Exception 
     */
    public void closeEPICSCA() throws Exception {
        logMessage("Closing EPICS", true, true);
        ctxt.destroy();
    }

    /**
     * This method takes a string argument, creates a Channel object, and
     * returns it.
     * 
     * @param chname      A string that is the name of the PV.
     * @return
     * @throws Exception 
     */
    public Channel createEPICSChannel(String chname) throws Exception {
        // Create the Channel to connect to the PV.
        Channel ch = ctxt.createChannel(chname);

        // send the request and wait for the channel to connect to the PV.
        ctxt.pendIO(2.0);
        if (isDebugFile) {
            debugPrintStream.print("\n\n  Channel info****************************\n");
            ch.printInfo(debugPrintStream);
        }
        if (isDebugMessages) {
            IJ.log("Host is " + ch.getHostName());
            IJ.log("can read = " + ch.getReadAccess());
            IJ.log("can write " + ch.getWriteAccess());
            IJ.log("type " + ch.getFieldType());
            IJ.log("name = " + ch.getName());
            IJ.log("element count = " + ch.getElementCount());
        }
        return (ch);
    }

    /**
     * This method is called when ever the  image plugin UniqueId_RBV process
     * variable changes value.
     */
    public class newUniqueIdCallback implements MonitorListener {

        @Override
        public void monitorChanged(MonitorEvent ev) {
            if (isDebugMessages) {
                IJ.log("Monitor callback");
            }
            isNewImageAvailable = true;
            DBR_Int x = (DBR_Int) ev.getDBR();
            UniqueId = (x.getIntValue())[0];
            // I'd like to just do the synchronized notify here, but how do I get "this"?
            newUniqueId(ev);
        }
    }

    
    /**
     * This method is called whenever the size of the data array in the X
     * dimension changes.  It reads the X value of the start of the camera's
     * readout and the size of the data array and saves those values.
     */
    public class newCameraArraySizeX implements MonitorListener {
        
        @Override
        public void monitorChanged(MonitorEvent ev) {
            if (isDebugMessages)
                IJ.log("New size for " + cameraPrefix + "ArraySizeX_RBV");
            
            DBR_Int sizeX = (DBR_Int) ev.getDBR();
            cameraArraySizeX = (sizeX.getIntValue())[0];
            
            try {
                ch_minCamX_RBV.get(DBRType.INT, 1, new GetListener() {
                
                    @Override
                    public void getCompleted (GetEvent ev) {
                        DBR_Int x = (DBR_Int) ev.getDBR();
                        cameraOriginX = (x.getIntValue())[0];
                        if (isDebugMessages)
                           IJ.log("Camera X origin at " + cameraOriginX);
                    }
                });
            } catch (CAException ex) {
                logMessage("newCameraArraySizeX CAException: Cannot read " + cameraPrefix + "MinX_RBV:" + ex.getMessage(), true, true);
            } catch (IllegalStateException ex) {
                logMessage("newCameraArraySizeX IllegalStateException: Cannot read " + cameraPrefix + "MinX_RBV:" + ex.getMessage(), true, true);
            }
            
            try {
                ch_camBinX_RBV.get(DBRType.INT, 1, new GetListener() {
                
                    @Override
                    public void getCompleted (GetEvent ev) {
                        DBR_Int x = (DBR_Int) ev.getDBR();
                        cameraBinX = (x.getIntValue())[0];
                        if (isDebugMessages)
                           IJ.log("Camera X bin value =  " + cameraBinX);
                    }
                });
            } catch (CAException ex) {
                logMessage("newCameraArraySizeX CAException: Cannot read " + cameraPrefix + "BinX_RBV:" + ex.getMessage(), true, true);
            } catch (IllegalStateException ex) {
                logMessage("newCameraArraySizeX IllegalStateException: Cannot read " + cameraPrefix + "BinX_RBV:" + ex.getMessage(), true, true);
            }
        }
    }
    
    /**
     * This method is called whenever the size of the data array in the Y
     * dimension changes.  It reads the Y value of the start of the camera's
     * readout and the size of the data array and saves those values.
     */
    public class newCameraArraySizeY implements MonitorListener {
        
        @Override
        public void monitorChanged(MonitorEvent ev) {
            if (isDebugMessages)
                IJ.log("New size for " + cameraPrefix + "ArraySizeY_RBV");
            
            DBR_Int sizeY = (DBR_Int) ev.getDBR();
            cameraArraySizeY = (sizeY.getIntValue())[0];
            
            try {
                ch_minCamY_RBV.get(DBRType.INT, 1, new GetListener() {
                
                    @Override
                    public void getCompleted (GetEvent ev) {
                        DBR_Int y = (DBR_Int) ev.getDBR();
                        cameraOriginY = (y.getIntValue())[0];
                        if (isDebugMessages)
                            IJ.log("Camera Y origin at " + cameraOriginY);
                    }
                });
            } catch (CAException ex) {
                logMessage("newCameraArraySizeY CAException: Cannot read " + cameraPrefix + "MinY_RBV:" + ex.getMessage(), true, true);
            } catch (IllegalStateException ex) {
                logMessage("newCameraArraySizeY IllegalStateException: Cannot read " + cameraPrefix + "MinY_RBV:" + ex.getMessage(), true, true);
            }
            
            try {
                ch_camBinY_RBV.get(DBRType.INT, 1, new GetListener() {
                
                    @Override
                    public void getCompleted (GetEvent ev) {
                        DBR_Int y = (DBR_Int) ev.getDBR();
                        cameraBinY = (y.getIntValue())[0];
                        if (isDebugMessages)
                            IJ.log("Camera Y bin value = " + cameraBinY);
                    }
                });
            } catch (CAException ex) {
                logMessage("newCameraArraySizeY CAException: Cannot read " + cameraPrefix + "BinY_RBV:" + ex.getMessage(), true, true);
            } catch (IllegalStateException ex) {
                logMessage("newCameraArraySizeY IllegalStateException: Cannot read " + cameraPrefix + "BinY_RBV:" + ex.getMessage(), true, true);
            }
        }
    }
    
    /**
     * This method is called whenever the size of the data array for the ROI
     * plugin in the X dimension changes.  It reads the X value of the start
     * of the ROI and the size of the data array and saves those values.
     */
    public class newRoiArraySizeX implements MonitorListener {
        
        @Override
        public void monitorChanged(MonitorEvent ev) {
            if (isDebugMessages)
                IJ.log("New size for " + roiPrefix + "ArraySizeX_RBV");
            
            DBR_Int sizeX = (DBR_Int) ev.getDBR();
            roiArraySizeX = (sizeX.getIntValue())[0];
            
            try {
                ch_minRoiX_RBV.get(DBRType.INT, 1, new GetListener() {
                
                    @Override
                    public void getCompleted (GetEvent ev) {
                        DBR_Int x = (DBR_Int) ev.getDBR();
                        roiOriginX = (x.getIntValue())[0];
                        if (isDebugMessages)
                           IJ.log("ROI X origin at " + roiOriginX);
                    }
                });
            } catch (CAException ex) {
                logMessage("newRoiArraySizeX CAException: Cannot read " + roiPrefix + "MinX_RBV:" + ex.getMessage(), true, true);
            } catch (IllegalStateException ex) {
                logMessage("newRoiArraySizeX IllegalStateException: Cannot read " + roiPrefix + "MinX_RBV:" + ex.getMessage(), true, true);
            }
            
            try {
                ch_roiBinX_RBV.get(DBRType.INT, 1, new GetListener() {
                
                    @Override
                    public void getCompleted (GetEvent ev) {
                        DBR_Int x = (DBR_Int) ev.getDBR();
                        roiBinX = (x.getIntValue())[0];
                        if (isDebugMessages)
                           IJ.log("ROI X bin value = " + roiBinX);
                    }
                });
            } catch (CAException ex) {
                logMessage("newRoiArraySizeX CAException: Cannot read " + roiPrefix + "BinX_RBV:" + ex.getMessage(), true, true);
            } catch (IllegalStateException ex) {
                logMessage("newRoiArraySizeX IllegalStateException: Cannot read " + roiPrefix + "BinX_RBV:" + ex.getMessage(), true, true);
            }
        }
    }
    
    /**
     * This method is called whenever the size of the data array for the ROI
     * plugin in the Y dimension changes.  It reads the Y value of the start
     * of the ROI and the size of the data array and saves those values.
     */
    public class newRoiArraySizeY implements MonitorListener {
        
        @Override
        public void monitorChanged(MonitorEvent ev) {
            if (isDebugMessages)
                IJ.log("New size for " + roiPrefix + "ArraySizeY_RBV");
            
            DBR_Int sizeY = (DBR_Int) ev.getDBR();
            roiArraySizeY = (sizeY.getIntValue())[0];
            
            try {
                ch_minRoiY_RBV.get(DBRType.INT, 1, new GetListener() {
                
                    @Override
                    public void getCompleted (GetEvent ev) {
                        DBR_Int y = (DBR_Int) ev.getDBR();
                        roiOriginY = (y.getIntValue())[0];
                        if (isDebugMessages)
                           IJ.log("ROI Y origin at " + roiOriginY);
                    }
                });
            } catch (CAException ex) {
                logMessage("newRoiArraySizeY CAException: Cannot read " + roiPrefix + "MinY_RBV:" + ex.getMessage(), true, true);
            } catch (IllegalStateException ex) {
                logMessage("newRoiArraySizeY IllegalStateException: Cannot read " + roiPrefix + "MinY_RBV:" + ex.getMessage(), true, true);
            }
            
            try {
                ch_roiBinY_RBV.get(DBRType.INT, 1, new GetListener() {
                
                    @Override
                    public void getCompleted (GetEvent ev) {
                        DBR_Int x = (DBR_Int) ev.getDBR();
                        roiBinY = (x.getIntValue())[0];
                        if (isDebugMessages)
                           IJ.log("ROI Y bin value = " + roiBinX);
                    }
                });
            } catch (CAException ex) {
                logMessage("newRoiArraySizeY CAException: Cannot read " + roiPrefix + "BinY_RBV:" + ex.getMessage(), true, true);
            } catch (IllegalStateException ex) {
                logMessage("newRoiArraySizeY IllegalStateException: Cannot read " + roiPrefix + "BinY_RBV:" + ex.getMessage(), true, true);
            }
        }
    }

    /**
     * This method wakes up the main thread to get a new image.
     * 
     * @param ev    An event object that represents the the source of the event.
     */
    public void newUniqueId(MonitorEvent ev) {
        synchronized (this) {
            notify();
        }
    }

    /**
     * This method checks that the PV objects for the image plugin both exist
     * and are connected to the underlying PVs.
     */
    public void checkImagePVConnections() {
        boolean imageConnected;
        
        try {
            imageConnected = (ch_nx != null && ch_nx.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_ny != null && ch_ny.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_nz != null && ch_nz.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_colorMode != null && ch_colorMode.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_image != null && ch_image.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_image_id != null && ch_image_id.getConnectionState() == Channel.ConnectionState.CONNECTED);

            if (imageConnected & !isImageConnected) {
                isImageConnected = true;
                logMessage("Connection to EPICS image PVs OK", true, true);
                imagePrefixText.setBackground(Color.green);
                startButton.setEnabled(!isDisplayImages);
                stopButton.setEnabled(isDisplayImages);
                snapButton.setEnabled(isDisplayImages);
            }
            if (!imageConnected) {
                isImageConnected = false;
                logMessage("Cannot connect to EPICS image PVs", true, true);
                imagePrefixText.setBackground(Color.red);
                startButton.setEnabled(false);
                stopButton.setEnabled(false);
                snapButton.setEnabled(false);
            }
            
        } catch (IllegalStateException ex) {
            IJ.log("checkImagePVConnections: got exception= " + ex.getMessage());
            if (isDebugFile) {
                ex.printStackTrace(debugPrintStream);
            }
        }
    }
    
    /**
     * This method checks that the PV objects for the camera both exist
     * and are connected to the underlying PVs.
     */
    public void checkCameraPVConnections() {
        boolean cameraConnected;
        try {
            cameraConnected = (ch_minCamX != null && ch_minCamX.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_minCamY != null && ch_minCamY.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_minCamX_RBV != null && ch_minCamX_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_minCamY_RBV != null && ch_minCamY_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_sizeCamX != null && ch_sizeCamX.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_sizeCamY != null && ch_sizeCamY.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_maxSizeCamX != null && ch_maxSizeCamX.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_maxSizeCamY != null && ch_maxSizeCamY.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_camBinX_RBV != null && ch_camBinX_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_camBinY_RBV != null && ch_camBinY_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_sizeCamArrayX_RBV != null && ch_sizeCamArrayX_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_sizeCamArrayY_RBV != null && ch_sizeCamArrayY_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED);

            if (cameraConnected & !isCameraConnected) {
                isCameraConnected = true;
                logMessage("Connection to EPICS camera PVs OK", true, true);
                cameraPrefixText.setBackground(Color.green);
                setCCDReadoutRadioButton.setEnabled(true);
                resetRoiButton.setEnabled(true);
                
                cameraOriginX = epicsGetInt(ch_minCamX_RBV);
                cameraOriginY = epicsGetInt(ch_minCamY_RBV);
                cameraArraySizeX = epicsGetInt(ch_sizeCamArrayX_RBV);
                cameraArraySizeY = epicsGetInt(ch_sizeCamArrayY_RBV);
                cameraBinX = epicsGetInt(ch_camBinX_RBV);
                cameraBinY = epicsGetInt(ch_camBinY_RBV);
                
            }

            if (!cameraConnected) {
                isCameraConnected = false;
                logMessage("Cannot connect to EPICS camera PVs", true, true);
                cameraPrefixText.setBackground(Color.red);
                setCCDReadoutRadioButton.setSelected(false);
                setCCDReadoutRadioButton.setEnabled(false);
                resetRoiButton.setEnabled(false);
                setCCD_Readout = false;
            }


        } catch (IllegalStateException ex) {
            IJ.log("checkCameraPVConnections: got IllegalStateException= " + ex.getMessage());
            if (isDebugFile) {
                ex.printStackTrace(debugPrintStream);
            }
        } catch (TimeoutException ex) {
            IJ.log("checkCameraPVConnections: got TimeoutException= " + ex.getMessage());
            if (isDebugFile) {
                ex.printStackTrace(debugPrintStream);
            }
        } catch (CAException ex) {
            IJ.log("checkCameraPVConnections: got CAException= " + ex.getMessage());
            if (isDebugFile) {
                ex.printStackTrace(debugPrintStream);
            }
        }
    }
    
    /**
     * This method checks that the PV objects for the transform plugin both exist
     * and are connected to the underlying PVs.
     */
    public void checkTransformPVConnections() {
        boolean transformConnected;
        
        try {
            transformConnected = (ch_transType[0] != null && ch_transType[0].getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_transType[1] != null && ch_transType[1].getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_transType[2] != null && ch_transType[2].getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_transType[3] != null && ch_transType[3].getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_origin != null && ch_origin.getConnectionState() == Channel.ConnectionState.CONNECTED);

            if (transformConnected & !isTransformConnected) {
                isTransformConnected = true;
                logMessage("Connection to EPICS transform PVs OK", true, true);
                transformPrefixText.setBackground(Color.green);
                useTransformCheckBox.setEnabled(true);
            }

            if (!transformConnected) {
                isTransformConnected = false;
                logMessage("Cannot connect to EPICS transform PVs", true, true);
                transformPrefixText.setBackground(Color.red);
                useTransformCheckBox.setSelected(false);
                useTransformCheckBox.setEnabled(false);
                useTransformPlugin = false;
            }

        } catch (IllegalStateException ex) {
            IJ.log("checkTransformPVConnections: got exception= " + ex.getMessage());
            if (isDebugFile) {
                ex.printStackTrace(debugPrintStream);
            }
        }
    }
    
    /**
     * This method checks that the PV objects for the ROI plugin both exist
     * and are connected to the underlying PVs.
     */
    public void checkRoiPVConnections() {
        boolean roiConnected;

        try {
            roiConnected = (ch_minRoiX != null && ch_minRoiX.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_minRoiY != null && ch_minRoiY.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_minRoiX_RBV != null && ch_minRoiX_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_minRoiY_RBV != null && ch_minRoiY_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_sizeRoiX != null && ch_sizeRoiX.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_sizeRoiY != null && ch_sizeRoiY.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_roiBinX_RBV != null && ch_roiBinX_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_roiBinY_RBV != null && ch_roiBinY_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_sizeRoiArrayX_RBV != null && ch_sizeRoiArrayX_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_sizeRoiArrayY_RBV != null && ch_sizeRoiArrayY_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED);

            if (roiConnected & !isRoiConnected) {
                isRoiConnected = true;
                logMessage("Connection to EPICS ROI PVs OK", true, true);
                roiPrefixText.setBackground(Color.green);
                setROIRadioButton.setEnabled(true);
                preTransformRadioButton.setEnabled(true);
                postTransformRadioButton.setEnabled(true);
                
                roiOriginX = epicsGetInt(ch_minRoiX_RBV);
                roiOriginY = epicsGetInt(ch_minRoiY_RBV);
                roiArraySizeX = epicsGetInt(ch_sizeRoiArrayX_RBV);
                roiArraySizeY = epicsGetInt(ch_sizeRoiArrayY_RBV);
                roiBinX = epicsGetInt(ch_roiBinX_RBV);
                roiBinY = epicsGetInt(ch_roiBinY_RBV);
            }

            if (!roiConnected) {
                isRoiConnected = false;
                logMessage("Cannot connect to EPICS ROI PVs", true, true);
                roiPrefixText.setBackground(Color.red);
                setROIRadioButton.setSelected(false);
                setROIRadioButton.setEnabled(false);
                preTransformRadioButton.setEnabled(false);
                postTransformRadioButton.setEnabled(false);
            }
            
        } catch (IllegalStateException ex) {
            IJ.log("checkROIPVConnections: got exception= " + ex.getMessage());
            if (isDebugFile) {
                ex.printStackTrace(debugPrintStream);
            }
        } catch (TimeoutException ex) {
            IJ.log("checkROIPVConnections: got exception= " + ex.getMessage());
            if (isDebugFile) {
                ex.printStackTrace(debugPrintStream);
            }
        } catch (CAException ex) {
            IJ.log("checkROIPVConnections: got exception= " + ex.getMessage());
            if (isDebugFile) {
                ex.printStackTrace(debugPrintStream);
            }
        }
    }

    /**
     * This method reads new data from the area detector's NDarrays.  If the
     * size of the arrays have changed, it closes the image window and creates
     * a new one.
     */
    public void updateImage() {
        try {
            checkImagePVConnections();
            if (!isImageConnected) {
                return;
            }
            int nx = epicsGetInt(ch_nx);
            int ny = epicsGetInt(ch_ny);
            int nz = epicsGetInt(ch_nz);
            int cm = epicsGetInt(ch_colorMode);
            DBRType dt = ch_image.getFieldType();

            if (nz == 0) {
                nz = 1;
            }
            int getsize = nx * ny * nz;
            if (getsize == 0) {
                return;  // Not valid dimensions
            }
            if (isDebugMessages) {
                IJ.log("got image, sizes: " + nx + " " + ny + " " + nz);
            }

            // if image size changes we must close window and make a new one.
            boolean makeNewWindow = false;
            if (nx != imageSizeX || ny != imageSizeY || nz != imageSizeZ || cm != colorMode || dt != dataType) {
                makeNewWindow = true;
                imageSizeX = nx;
                imageSizeY = ny;
                imageSizeZ = nz;
                colorMode = cm;
                dataType = dt;
                NXText.setText("" + imageSizeX);
                NYText.setText("" + imageSizeY);
                NZText.setText("" + imageSizeZ);
            }

            // If we are making a new stack close the window
            if (isNewStack) {
                makeNewWindow = true;
            }

            // If we need to make a new window then close the current one if it exists
            if (makeNewWindow) {
                try {
                    if (img.getWindow() == null || !img.getWindow().isClosed()) {
                        img.close();
                        canvas.removeMouseListener(this);
                    }
                } catch (Exception ex) {
                }
                makeNewWindow = false;
            }
            // If the window does not exist or is closed make a new one
            if (img.getWindow() == null || img.getWindow().isClosed()) {
                switch (colorMode) {
                    case 0:
                    case 1:
                        if (dataType.isBYTE()) {
                            img = new ImagePlus(imagePrefix, new ByteProcessor(imageSizeX, imageSizeY));
                        } else if (dataType.isSHORT()) {
                            img = new ImagePlus(imagePrefix, new ShortProcessor(imageSizeX, imageSizeY));
                        } else if (dataType.isINT() || dataType.isFLOAT() || dataType.isDOUBLE()) {
                            img = new ImagePlus(imagePrefix, new FloatProcessor(imageSizeX, imageSizeY));
                        }
                        break;
                    case 2:
                        img = new ImagePlus(imagePrefix, new ColorProcessor(imageSizeY, imageSizeZ));
                        break;
                    case 3:
                        img = new ImagePlus(imagePrefix, new ColorProcessor(imageSizeX, imageSizeZ));
                        break;
                    case 4:
                        img = new ImagePlus(imagePrefix, new ColorProcessor(imageSizeX, imageSizeY));
                        break;
                }
                img.show();

                win = img.getWindow();
                canvas = win.getCanvas();
                canvas.addMouseListener(this);
            }

            if (isNewStack) {
                imageStack = new ImageStack(img.getWidth(), img.getHeight());
                imageStack.addSlice(imagePrefix + UniqueId, img.getProcessor());
                // Note: we need to add this first slice twice in order to get the slider bar 
                // on the window - ImageJ won't put it there if there is only 1 slice.
                imageStack.addSlice(imagePrefix + UniqueId, img.getProcessor());
                img.close();
                img = new ImagePlus(imagePrefix, imageStack);
                img.show();
                isNewStack = false;
            }

            if (isDebugMessages) {
                IJ.log("about to get pixels");
            }
            if (colorMode == 0 || colorMode == 1) {
                if (dataType.isBYTE()) {
                    byte[] pixels = (byte[]) img.getProcessor().getPixels();
                    pixels = epicsGetByteArray(ch_image, getsize);
                    img.getProcessor().setPixels(pixels);
                } else if (dataType.isSHORT()) {
                    short[] pixels = (short[]) img.getProcessor().getPixels();
                    pixels = epicsGetShortArray(ch_image, getsize);
                    img.getProcessor().setPixels(pixels);
                } else if (dataType.isINT() || dataType.isFLOAT() || dataType.isDOUBLE()) {
                    float[] pixels = (float[]) img.getProcessor().getPixels();
                    pixels = epicsGetFloatArray(ch_image, getsize);
                    img.getProcessor().setPixels(pixels);
                }
            } else if (colorMode >= 2 && colorMode <= 4) {
                int[] pixels = (int[]) img.getProcessor().getPixels();
                byte inpixels[] = epicsGetByteArray(ch_image, getsize);
                switch (colorMode) {
                    case 2: {
                        int in = 0, out = 0;
                        while (in < getsize) {
                            pixels[out++] = (inpixels[in++] & 0xFF) << 16 | (inpixels[in++] & 0xFF) << 8 | (inpixels[in++] & 0xFF);
                        }
                    }
                    break;
                    case 3: {
                        int nCols = imageSizeX, nRows = imageSizeZ, row, col;
                        int redIn, greenIn, blueIn, out = 0;
                        for (row = 0; row < nRows; row++) {
                            redIn = row * nCols * 3;
                            greenIn = redIn + nCols;
                            blueIn = greenIn + nCols;
                            for (col = 0; col < nCols; col++) {
                                pixels[out++] = (inpixels[redIn++] & 0xFF) << 16 | (inpixels[greenIn++] & 0xFF) << 8 | (inpixels[blueIn++] & 0xFF);
                            }
                        }
                    }
                    break;
                    case 4: {
                        int imageSize = imageSizeX * imageSizeY;
                        int redIn = 0, greenIn = imageSize, blueIn = 2 * imageSize, out = 0;
                        while (redIn < imageSize) {
                            pixels[out++] = (inpixels[redIn++] & 0xFF) << 16 | (inpixels[greenIn++] & 0xFF) << 8 | (inpixels[blueIn++] & 0xFF);
                        }
                    }
                    break;
                }
                img.getProcessor().setPixels(pixels);
            }

            if (isSaveToStack) {
                img.getStack().addSlice(imagePrefix + UniqueId, img.getProcessor().duplicate());
            }
            img.setSlice(img.getNSlices());
            img.show();
            img.updateAndDraw();
            img.updateStatusbarValue();
            numImageUpdates++;

        } catch (CAException ex) {
            IJ.log("UpdateImage got CAException: " + ex.getMessage());
            if (isDebugFile) {
                ex.printStackTrace(debugPrintStream);
            }
        } catch (TimeoutException ex) {
            IJ.log("UpdateImage got TimeoutException: " + ex.getMessage());
            if (isDebugFile) {
                ex.printStackTrace(debugPrintStream);
            }
        } catch (IllegalStateException ex) {
            IJ.log("UpdateImage got IllegalStatexception: " + ex.getMessage());
            if (isDebugFile) {
                ex.printStackTrace(debugPrintStream);
            }
        }
    }

    /**
     * Read an integer value from a PV and return it.
     * 
     * @param ch    A channel object that is used to read the PV.
     * @return
     * @throws gov.aps.jca.TimeoutException
     * @throws gov.aps.jca.CAException
     * @throws IllegalStateException
     */
    public int epicsGetInt(Channel ch) throws TimeoutException, CAException, IllegalStateException {
        if (isDebugMessages) {
            IJ.log("Channel Get: " + ch.getName());
        }
        DBR_Int x = (DBR_Int) ch.get(DBRType.INT, 1);
        ctxt.pendIO(5.0);
        return (x.getIntValue()[0]);
    }

    /**
     * Read an enum (short int) value from a PV and return it.
     * 
     * @param ch   A channel object that is used to read the PV.
     * @return
     * @throws gov.aps.jca.TimeoutException
     * @throws gov.aps.jca.CAException
     * @throws IllegalStateException 
     */
    public short epicsGetEnum(Channel ch) throws TimeoutException, CAException, IllegalStateException {
        if (isDebugMessages) {
            IJ.log("Channel Get: " + ch.getName());
        }
        DBR_Enum x = (DBR_Enum) ch.get(DBRType.ENUM, 1);
        ctxt.pendIO(5.0);
        return (x.getEnumValue()[0]);
    }

    /**
     * Read an array of byte values from a PV and return it.
     * 
     * @param ch    A channel object that is used to read the PV.
     * @param num   The number of bytes to read.
     * @return
     * @throws gov.aps.jca.TimeoutException
     * @throws gov.aps.jca.CAException
     * @throws IllegalStateException 
     */
    public byte[] epicsGetByteArray(Channel ch, int num) throws TimeoutException, CAException, IllegalStateException {
        DBR x = ch.get(DBRType.BYTE, num);
        ctxt.pendIO(10.0);
        DBR_Byte xi = (DBR_Byte) x;
        byte zz[] = xi.getByteValue();
        return (zz);
    }

    /**
     * Read an array of short integers from a PV and return it.
     * 
     * @param ch    A channel object that is used to read the PV.
     * @param num   The number of short integers to read.
     * @return
     * @throws gov.aps.jca.TimeoutException
     * @throws gov.aps.jca.CAException
     * @throws IllegalStateException 
     */
    public short[] epicsGetShortArray(Channel ch, int num) throws TimeoutException, CAException, IllegalStateException {
        DBR x = ch.get(DBRType.SHORT, num);
        ctxt.pendIO(10.0);
        DBR_Short xi = (DBR_Short) x;
        short zz[] = xi.getShortValue();
        return (zz);
    }

    /**
     * Read an array of float values from a PV and return it.
     * 
     * @param ch    A channel object used to read the PV.
     * @param num   The number of float values to read.
     * @return
     * @throws gov.aps.jca.TimeoutException
     * @throws gov.aps.jca.CAException
     * @throws IllegalStateException 
     */
    public float[] epicsGetFloatArray(Channel ch, int num) throws TimeoutException, CAException, IllegalStateException {
        DBR x = ch.get(DBRType.FLOAT, num);
        ctxt.pendIO(10.0);
        DBR_Float xi = (DBR_Float) x;
        float zz[] = xi.getFloatValue();
        return (zz);
    }

    /**
     * Write an integer value to a PV.
     * 
     * @param ch    A channel object used to write to the PV.
     * @param num   The integer value to be written.
     * @throws gov.aps.jca.TimeoutException
     * @throws gov.aps.jca.CAException
     * @throws IllegalStateException 
     */
    public void epicsSetInt(Channel ch, int num) throws TimeoutException, CAException, IllegalStateException {
        ch.put(num, new PutListener() {
            @Override
            public void putCompleted(PutEvent ev) {
                if (isDebugMessages)
                    IJ.log("Put operation complete");
            }
        });
        ctxt.pendIO(5.0);
    }

    /**
     * This method writes the values for the user drawn rectangle to either the
     * camera of the ROI plugin.
     * 
     * @param chX      This is the PV for the X value of the rectangle's origin.
     * @param chY      This is the PV for the Y value of the rectangle's origin.
     * @param chSizeX  This is the PV for the size of the rectangle in X.
     * @param chSizeY  This is the PV for the size of the rectangle in Y.
     * @param x        This is the value to write to chX.
     * @param y        This is the value to write to chY.
     * @param sizeX    This is the value to write to chSizeX.
     * @param sizeY    This is the value to write to chSizeY.
     */
    public void setROI(Channel chX, Channel chY, Channel chSizeX, Channel chSizeY,
            int x, int y, int sizeX, int sizeY) {
        try {
            epicsSetInt(chX, x);           
            epicsSetInt(chY, y);
            epicsSetInt(chSizeX, sizeX);
            epicsSetInt(chSizeY, sizeY);
        } catch (CAException ex) {
            logMessage("setROI got CAException: " + ex.getMessage(), true, true);
        } catch (TimeoutException ex) {
            logMessage("setROI got TimeoutException: " + ex.getMessage(), true, true);
        } catch (IllegalStateException ex) {
            logMessage("setROI got IllegalStateException: " + ex.getMessage(), true, true);
        }
    }

    /**
     * This class is used to determine the location of the origin point
     * and the orientation of the X and Y axes.
     */
    private class OriginParameters {

        private short origin;
        private short[] transformType = new short[4];
        private boolean flipAxes;
        private int ii;

        public OriginParameters() {
            try {
                //origin = epicsGetEnum(ch_origin);
                origin = UPPER_LEFT;
                flipAxes = false;

                for (ii = 0; ii < MAX_TRANSFORMS; ii++) {
                    transformType[ii] = epicsGetEnum(ch_transType[ii]);
                }
            } catch (CAException ex) {
                IJ.log("OriginParameters CAException: Could not get parameters from " + transformPrefix + ": " + ex.getMessage());
            } catch (TimeoutException ex) {
                IJ.log("OriginParameters TimeoutException: Could not get parameters from " + transformPrefix + ": " + ex.getMessage());
            } catch (IllegalStateException ex) {
                IJ.log("OriginParameters IllegalStateException: Could not get parameters from " + transformPrefix + ": " + ex.getMessage());
            }
        }

        /**
         * This method determines where the origin of the image is with respect to
         * what is displayed by this plugin.  It also determines whether or not the
         * X and Y axes have been flipped so that the X axis is now vertical and the
         * Y axis is now horizontal.
         * <p>
         * It iterates through all of the transforms types that the area detector
         * Transform plugin performs and moves the origin location and axis
         * orientation accordingly.  The results can then be obtained through two
         * get methods.
         * 
         * @see getOrigin()
         * @see isFlippedAxes()
         */
        public void findOriginLocation() {
            for (ii = 0; ii < MAX_TRANSFORMS; ii++) {
                switch (transformType[ii]) {
                    
                    // This image is not altered by the transform.
                    case NONE:
                        break;

                    // Rotate the image 90 degrees clockwise.
                    case ROTATE_90_CW:

                        switch (origin) {
                            case LOWER_LEFT:
                                origin = UPPER_LEFT;
                                break;
                            case UPPER_LEFT:
                                origin = UPPER_RIGHT;
                                break;
                            case UPPER_RIGHT:
                                origin = LOWER_RIGHT;
                                break;
                            case LOWER_RIGHT:
                                origin = LOWER_LEFT;
                                break;
                            default:
                                logMessage("findOriginLocation: found no matching origin location", true, true);
                                break;
                        }

                        flipAxes = !flipAxes;
                        break;

                    // Rotate the image 90 degrees counter clockwise.
                    case ROTATE_90_CCW:

                        switch (origin) {
                            case LOWER_LEFT:
                                origin = LOWER_RIGHT;
                                break;
                            case UPPER_LEFT:
                                origin = LOWER_LEFT;
                                break;
                            case UPPER_RIGHT:
                                origin = UPPER_LEFT;
                                break;
                            case LOWER_RIGHT:
                                origin = UPPER_RIGHT;
                                break;
                            default:
                                logMessage("findOriginLocation: found no matching origin location", true, true);
                                break;
                        }

                        flipAxes = !flipAxes;
                        break;

                    // Rotate the image 180 degrees.
                    case ROTATE_180:

                        switch (origin) {
                            case LOWER_LEFT:
                                origin = UPPER_RIGHT;
                                break;
                            case UPPER_LEFT:
                                origin = LOWER_RIGHT;
                                break;
                            case UPPER_RIGHT:
                                origin = LOWER_LEFT;
                                break;
                            case LOWER_RIGHT:
                                origin = UPPER_LEFT;
                                break;
                            default:
                                logMessage("findOriginLocation: found no matching origin location", true, true);
                                break;
                        }
                        break;

                    // Flip the image along the diagonal that goes from 0,0 to
                    // maxX, maxY.  This does not cause the origin location to
                    // change.
                    case FLIP_0011:

                        flipAxes = !flipAxes;
                        break;

                    // Flip the image along the diagonal that goes from 0, maxY
                    // to maxX, 0.
                    case FLIP_0110:

                        switch (origin) {
                            case LOWER_LEFT:
                                origin = UPPER_RIGHT;
                                break;
                            case UPPER_LEFT:
                                origin = LOWER_RIGHT;
                                break;
                            case UPPER_RIGHT:
                                origin = LOWER_LEFT;
                                break;
                            case LOWER_RIGHT:
                                origin = UPPER_LEFT;
                                break;
                            default:
                                logMessage("findOriginLocation: found no matching origin location", true, true);
                                break;
                        }

                        flipAxes = !flipAxes;
                        break;

                    // Flip x values of the image pixels.
                    case FLIP_X:

                        switch (origin) {
                            case LOWER_LEFT:
                                origin = LOWER_RIGHT;
                                break;
                            case UPPER_LEFT:
                                origin = UPPER_RIGHT;
                                break;
                            case UPPER_RIGHT:
                                origin = UPPER_LEFT;
                                break;
                            case LOWER_RIGHT:
                                origin = LOWER_LEFT;
                                break;
                            default:
                                logMessage("findOriginLocation: found no matching origin location", true, true);
                                break;
                        }
                        break;

                    // Flip the y values of the image pixels.
                    case FLIP_Y:

                        switch (origin) {
                            case LOWER_LEFT:
                                origin = UPPER_LEFT;
                                break;
                            case UPPER_LEFT:
                                origin = LOWER_LEFT;
                                break;
                            case UPPER_RIGHT:
                                origin = LOWER_RIGHT;
                                break;
                            case LOWER_RIGHT:
                                origin = UPPER_RIGHT;
                                break;
                            default:
                                logMessage("findOriginLocation: found no matching origin location", true, true);
                                break;
                        }
                        break;

                    default:
                        logMessage("findOriginLocation: found no matching transform type", true, true);
                        break;
                }
            }
            if (isDebugMessages)
                IJ.log("findOriginLocation: origin location is " + origin);
        }

        public short getOrigin() {
            return origin;
        }

        public boolean isFlippedAxes() {
            return flipAxes;
        }
    }

    /**
     * This method calculates the values that will be written to the area detector
     * software.  It calculates a starting point for the X and Y coordinates based
     * on the location of the origin point with respect to the image.  If the user
     * has checked the use Transform Plugin checkbox, then it calculates the origin
     * based on the AD array transforms performed.  It will also determine if the X
     * and Y axes have been flipped.  Otherwise, it assumes that the origin is at
     * the upper right corner of the image and the axes have not been flipped.  It
     * will write its values to either the cameras readout regions PVs, or to a ROI
     * plugin, depending on the user's choice.
     * 
     * @param startX    the starting point in X of the rectangle drawn on the image
     * @param startY    the starting point in X of the rectangle drawn on the image
     * @param sizeX     the size of the rectangle in the X dimension
     * @param sizeY     the size of the rectangle in the Y dimension
     * @param maxSizeX  the maximum size of the current image in the X dimension
     * @param maxSizeY  the maximum size of the current image in the Y dimension
     */
    public void calculateROI(int startX, int startY, int sizeX, int sizeY, int maxSizeX, int maxSizeY) {
        short origin = UPPER_LEFT;
        boolean flipAxes = false;
        OriginParameters params;
        
        int tempX = 0;
        int tempY = 0;
        int tempSizeX = 0;
        int tempSizeY = 0;

        if (useTransformPlugin  && !((setROI) && (isPostTransform)))
        {
            params = this.new OriginParameters();
            params.findOriginLocation();
            origin = params.getOrigin();
            flipAxes = params.isFlippedAxes();
            
            if (isDebugMessages) {
                IJ.log("The origin=" + origin);
                IJ.log("Axes are flipped=" + flipAxes);
            }
        }
        
        
        
        /* If the user wanted to set the ROI plugin and
         * the ROI gets its data from the transform
         * plugin, then the axes should not be flipped.
         */
        /*if (setROI && isPostTransform)
            flipAxes = false;*/
        
        switch (origin)
        {
            case LOWER_LEFT:
                if (flipAxes)
                {
                    tempX = maxSizeX - (startY + sizeY);
                    tempY = startX;
                    tempSizeX = sizeY;
                    tempSizeY = sizeX;
                }
                else
                {
                    tempX = startX;
                    tempY = maxSizeY - (startY + sizeY);
                    tempSizeX = sizeX;
                    tempSizeY = sizeY;
                }
                break;
                
            case UPPER_LEFT:
                if (flipAxes)
                {
                    tempX = startY;
                    tempY = startX;
                    tempSizeX = sizeY;
                    tempSizeY = sizeX;
                }
                else
                {
                    tempX = startX;
                    tempY = startY;
                    tempSizeX = sizeX;
                    tempSizeY = sizeY;
                }
                break;
                
            case LOWER_RIGHT:
                if (flipAxes)
                {
                    tempX = maxSizeX - (startY + sizeY);
                    tempY = maxSizeY - (startX + sizeX);
                    tempSizeX = sizeY;
                    tempSizeY = sizeX;
                }
                else
                {
                    tempX = maxSizeX - (startX + sizeX);
                    tempY = maxSizeY - (startY + sizeY);
                    tempSizeX = sizeX;
                    tempSizeY = sizeY;
                }
                break;

                
            case UPPER_RIGHT:
                if (flipAxes)
                {
                    tempX = startY;
                    tempY = maxSizeY - (startX + sizeX);
                    tempSizeX = sizeY;
                    tempSizeY = sizeX;
                }
                else
                {
                    tempX = maxSizeX - (startX + sizeX);
                    tempY = startY;
                    tempSizeX = sizeX;
                    tempSizeY = sizeY;
                }
                break;
                            
            default:
                break;
        }
        
        if (isDebugMessages) {
            IJ.log("The new calculated value of tempX is " + tempX);
            IJ.log("The new calculated value of tempY is " + tempY);
        }
                
        // Check to make sure that the sizes are not less than zero.
        if (tempSizeX < 0)
            tempSizeX = 0;
        
        if (tempSizeY < 0)
            tempSizeY = 0;
        
        try {
            if (setCCD_Readout) {
                if (isDebugMessages) {
                    IJ.log("The value of cameraOriginX is " + cameraOriginX);
                    IJ.log("The value of cameraOriginY is " + cameraOriginY);
                }
                
                // The user may have entered a value for binning.
                // Calulate the new value of tempX  and tempSizeX with binning.
                tempX = ((tempX - 1) * cameraBinX) + 1;
                tempSizeX = tempSizeX * cameraBinX;
                tempY = ((tempY - 1) * cameraBinY) + 1;
                tempSizeY = tempSizeY * cameraBinY;
                
                // Check to make sure that the values are not less than zero.
                if (tempX < 0)
                    tempX = 0;
        
                if (tempY < 0)
                    tempY = 0;

                tempX = tempX + cameraOriginX;
                tempY = tempY + cameraOriginY;
                
                setROI(ch_minCamX, ch_minCamY, ch_sizeCamX, ch_sizeCamY, tempX,
                            tempY, tempSizeX, tempSizeY);
            }
            
            if (setROI) {
                if (isDebugMessages) {
                    IJ.log("The value of roiOriginX is " + roiOriginX);
                    IJ.log("The value of roiOriginY is " + roiOriginY);
                }
                
                // The user may have entered a value for binning.
                // Calulate the new value of tempX  and tempSizeX with binning.
                tempX = ((tempX - 1) * roiBinX) + 1;
                tempSizeX = tempSizeX * roiBinX;
                tempY = ((tempY - 1) * roiBinY) + 1;
                tempSizeY = tempSizeY * roiBinY;
                
                // Check to make sure that the values are not less than zero.
                if (tempX < 0)
                    tempX = 0;
        
                if (tempY < 0)
                    tempY = 0;

                tempX = tempX + roiOriginX;
                tempY = tempY + roiOriginY;
                
                setROI(ch_minRoiX, ch_minRoiY, ch_sizeRoiX, ch_sizeRoiY, tempX,
                            tempY, tempSizeX, tempSizeY);
            }
        } catch (Exception ex) {
            logMessage("calculateROI: cannot get origin X and Y " + origin, true, true);
        }
        if (isDebugMessages) {
            IJ.log("The final calculated value of tempX is " + tempX);
            IJ.log("The final calculated value of tempY is " + tempY);
        }
    }

    /**
     * Create the GUI and show it. For thread safety, this method should be
     * invoked from the event-dispatching thread.
     */
    public void createAndShowGUI() {
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

        imagePrefixText = new JTextField(imagePrefix, 15);
        cameraPrefixText = new JTextField(cameraPrefix, 15);
        transformPrefixText = new JTextField(transformPrefix, 15);
        roiPrefixText = new JTextField(roiPrefix, 15);
        startButton = new JButton("Start");
        stopButton = new JButton("Stop");
        stopButton.setEnabled(false);
        snapButton = new JButton("Snap");
        resetRoiButton = new JButton("Reset CCD");
        JCheckBox captureCheckBox = new JCheckBox("");
        setNoneRadioButton = new JRadioButton("Set None",true);
        setCCDReadoutRadioButton = new JRadioButton("Set CCD Readout", false);
        setROIRadioButton = new JRadioButton("Set ROI Plugin", false);
        preTransformRadioButton = new JRadioButton("ROI Pre-Transform", false);
        preTransformRadioButton.setEnabled(false);
        postTransformRadioButton = new JRadioButton("ROI Post-Transform", false);
        postTransformRadioButton.setEnabled(false);
        useTransformCheckBox = new JCheckBox("Use Transform Plugin?", false);
        ButtonGroup groupA = new ButtonGroup();
        ButtonGroup groupB = new ButtonGroup();
        
        groupA.add(setNoneRadioButton);
        groupA.add(setCCDReadoutRadioButton);
        groupA.add(setROIRadioButton);
        
        groupB.add(preTransformRadioButton);
        groupB.add(postTransformRadioButton);
        
        JPanel groupAPanel = new JPanel();
        groupAPanel.setLayout(new BoxLayout(groupAPanel, BoxLayout.Y_AXIS));
        groupAPanel.add(setNoneRadioButton);
        groupAPanel.add(setCCDReadoutRadioButton);
        groupAPanel.add(setROIRadioButton);
        
        JPanel groupBPanel = new JPanel();
        groupBPanel.setLayout(new BoxLayout(groupBPanel, BoxLayout.Y_AXIS));
        groupBPanel.add(preTransformRadioButton);
        groupBPanel.add(postTransformRadioButton);
        
        frame = new JFrame("Image J EPICS_AD_Viewer Plugin");
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
        panel.add(new JLabel("Image PV Prefix"), c);
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

        // Second row
        // These widgets should be centered
        c.anchor = GridBagConstraints.CENTER;
        c.gridy = 1;
        c.gridx = 0;
        panel.add(imagePrefixText, c);
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

        // Third row
        c.anchor = GridBagConstraints.CENTER;
        c.gridy = 2;
        c.gridx = 0;
        panel.add(new JLabel("Camera PV Prefix"), c);
        c.gridx = 1;
        panel.add(new JLabel("ROI PV Prefix"), c);
        c.gridx = 2;
        panel.add(new JLabel("Transform PV Prefix"), c);

        // Fourth row
        c.anchor = GridBagConstraints.CENTER;
        c.gridy = 3;
        c.gridx = 0;
        panel.add(cameraPrefixText, c);
        c.gridx = 1;
        panel.add(roiPrefixText, c);
        c.gridx = 2;
        panel.add(transformPrefixText, c);
        c.gridx = 3;
        panel.add(resetRoiButton, c);

        // Fifth row
        c.anchor = GridBagConstraints.CENTER;
        c.gridy = 4;
        c.gridx = 0;
        panel.add(groupAPanel, c);
        c.gridx = 1;
        panel.add(groupBPanel, c);
        c.gridx = 2;
        panel.add(useTransformCheckBox, c);

        // Bottom row
        c.gridy = 5;
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
        timer = new javax.swing.Timer(timerDelay, new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent event) {
                checkImagePVConnections();
                long time = new Date().getTime();
                double fps = 1000. * numImageUpdates / (double) (time - prevTime);
                NumberFormat form = DecimalFormat.getInstance();
                ((DecimalFormat) form).applyPattern("0.0");
                FPSText.setText("" + form.format(fps));
                if (isDisplayImages && numImageUpdates > 0) {
                    logMessage("New images=" + numImageUpdates, true, false);
                }
                prevTime = time;
                numImageUpdates = 0;
            }
        });
        timer.start();

        imagePrefixText.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent event) {
                if (isDebugMessages)
                    IJ.log("Image prefix changed");
                disconnectImagePVs();
                connectImagePVs();
            }
        });

        cameraPrefixText.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent event) {
                if (isDebugMessages)
                    IJ.log("Camera prefix changed");
                disconnectCameraPVs();
                connectCameraPVs();
            }
        });

        transformPrefixText.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent event) {
                if (isDebugMessages)
                    IJ.log("Transform prefix changed");
                disconnectTransformPVs();
                connectTransformPVs();
            }
        });

        roiPrefixText.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent event) {
                if (isDebugMessages)
                    IJ.log("ROI prefix changed");
                disconnectRoiPVs();
                connectRoiPVs();
            }
        });

        startButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent event) {
                startButton.setEnabled(false);
                stopButton.setEnabled(true);
                snapButton.setEnabled(true);
                isDisplayImages = true;
                logMessage("Image display started", true, true);
            }
        });

        stopButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent event) {
                startButton.setEnabled(true);
                stopButton.setEnabled(false);
                snapButton.setEnabled(false);
                isDisplayImages = false;
                logMessage("Image display stopped", true, true);
            }
        });

        snapButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent event) {
                makeImageCopy();
            }
        });

        resetRoiButton.addActionListener(new ActionListener() {
            /**
             * This method resets both the camera readout region and the
             * plugin ROI to be the full size of the CCD.
             * 
             * @param event 
             */
            @Override
            public void actionPerformed(ActionEvent event) {
                int sizeX;
                int sizeY;

                try {
                    //Get the maximum size of the CCD
                    sizeX = epicsGetInt(ch_maxSizeCamX);
                    sizeY = epicsGetInt(ch_maxSizeCamY);
                    
                    //Reset the camera.
                    if (isCameraConnected)
                        setROI(ch_minCamX, ch_minCamY, ch_sizeCamX, ch_sizeCamY, 0, 0, sizeX, sizeY);
                    //Reset the ROI plugin.
                    if (isRoiConnected)
                        setROI(ch_minRoiX, ch_minRoiY, ch_sizeRoiX, ch_sizeRoiY, 0, 0, sizeX, sizeY);
                    
                } catch (CAException ex) {
                    IJ.log("CAException: Could not reset image ROI to full: " + ex.getMessage());
                } catch (TimeoutException ex) {
                    IJ.log("TimeoutException: Could not reset image ROI to full: " + ex.getMessage());
                } catch (IllegalStateException ex) {
                    IJ.log("IllegalStateException: Could not reset image ROI to full: " + ex.getMessage());
                }
                
            }
        });

        captureCheckBox.addItemListener(new ItemListener() {
            @Override
            public void itemStateChanged(ItemEvent e) {
                if (e.getStateChange() == ItemEvent.SELECTED) {
                    isSaveToStack = true;
                    isNewStack = true;
                    IJ.log("record on");
                } else {
                    isSaveToStack = false;
                    IJ.log("record off");
                }

            }
        }
        );
        
        setCCDReadoutRadioButton.addItemListener(new ItemListener () {
            @Override
            public void itemStateChanged(ItemEvent e) {
                setCCD_Readout = e.getStateChange() == ItemEvent.SELECTED;
                resetRoiButton.setText("Reset CCD");
            }
        });
        
        setROIRadioButton.addItemListener(new ItemListener () {
            @Override
            public void itemStateChanged(ItemEvent e) {
                setROI = e.getStateChange() == ItemEvent.SELECTED;
                preTransformRadioButton.setEnabled(setROI);
                postTransformRadioButton.setEnabled(setROI);
                resetRoiButton.setText("Reset ROI");
            }
        });
                
        preTransformRadioButton.addItemListener(new ItemListener () {
            @Override
            public void itemStateChanged(ItemEvent e) {
                isPreTransform = e.getStateChange() == ItemEvent.SELECTED;
            }
        });
        
        postTransformRadioButton.addItemListener(new ItemListener () {
            @Override
            public void itemStateChanged(ItemEvent e) {
                isPostTransform = (e.getStateChange() == ItemEvent.SELECTED);
            }
        });
        
        useTransformCheckBox.addItemListener(new ItemListener () {
            @Override
            public void itemStateChanged(ItemEvent e) {
                useTransformPlugin = (e.getStateChange() == ItemEvent.SELECTED);
            }
        });

        useTransformCheckBox.addItemListener(new ItemListener() {
            @Override
            public void itemStateChanged(ItemEvent e) {
                useTransformPlugin = (e.getStateChange() == ItemEvent.SELECTED);
            }
        }
        );

    }

    public class FrameExitListener extends WindowAdapter {

        @Override
        public void windowClosing(WindowEvent event) {
            isPluginRunning = false;
            isNewImageAvailable = false;
            // We need to wake up the main thread so it shuts down cleanly
            synchronized (this) {
                notify();
            }
        }
    }

    /**
     * This method creates a message based on the time and a supplied string.
     * It will optionally write the message to the ImageJ log and/or the
     * status display in the GUI.
     * 
     * @param message      A string that is the message to be displayed.
     * @param logDisplay   Indicates if the message should be displayed on the GUI.
     * @param logFile      Indicates if the message should be displayed on the ImageJ log.
     */
    public void logMessage(String message, boolean logDisplay, boolean logFile) {
        Date date = new Date();
        SimpleDateFormat simpleDate = new SimpleDateFormat("d/M/y k:m:s.S");
        String completeMessage;

        completeMessage = simpleDate.format(date) + ": " + message;
        if (logDisplay) {
            StatusText.setText(completeMessage);
        }
        if (logFile) {
            IJ.log(completeMessage);
        }
    }

    /**
     * This method reads values of PV prefixes from a file and writes them to
     * string variables.
     */
    public void readProperties() {
        String temp, path;

        try {
            String fileSep = System.getProperty("file.separator");
            path = System.getProperty("user.home") + fileSep + propertyFile;
            if (path == null) {
                throw new Exception("No such file: " + path);
            }
            FileInputStream file = new FileInputStream(path);
            properties.load(file);
            file.close();
            temp = properties.getProperty("imagePrefix");
            if (temp != null) {
                imagePrefix = temp;
            }
            temp = properties.getProperty("cameraPrefix");
            if (temp != null) {
                cameraPrefix = temp;
            }
            temp = properties.getProperty("transformPrefix");
            if (temp != null) {
                transformPrefix = temp;
            }
            temp = properties.getProperty("roiPrefix");
            if (temp != null) {
                roiPrefix = temp;
            }
            IJ.log("Read properties file: " + path + "  imagePrefix= " + imagePrefix);
        } catch (Exception ex) {
            IJ.log("readProperties:exception: " + ex.getMessage());
        }
    }

    /**
     * This method writes the prefixes of PVs to a file for later retrieval.
     */
    public void writeProperties() {
        String path;
        try {
            String fileSep = System.getProperty("file.separator");
            path = System.getProperty("user.home") + fileSep + propertyFile;
            properties.setProperty("imagePrefix", imagePrefix);
            properties.setProperty("cameraPrefix", cameraPrefix);
            properties.setProperty("transformPrefix", transformPrefix);
            properties.setProperty("roiPrefix", roiPrefix);
            FileOutputStream file = new FileOutputStream(path);
            properties.store(file, "EPICS_AD_Viewer Properties");
            file.close();
            IJ.log("Wrote properties file: " + path);
        } catch (IOException ex) {
            IJ.log("writeProperties:exception: " + ex.getMessage());
        }
    }
}
