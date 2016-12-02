// EPICS_AD_Controller.java
// Original authors
//      Tim Madden, APS
//      Chris Roehrig, APS
// Current author
//      Mark Rivers, University of Chicago

import ij.*;
import ij.process.*;
import java.awt.*;
import ij.plugin.*;
import ij.gui.ImageWindow;
import ij.gui.Roi;
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

public class EPICS_AD_Controller implements PlugIn {
    
    private static final short NONE = 0;
    private static final short ROTATE_90 = 1;
    private static final short ROTATE_180 = 2;
    private static final short ROTATE_270 = 3;
    private static final short MIRROR = 4;
    private static final short ROTATE_90_MIRROR = 5;
    private static final short ROTATE_180_MIRROR = 6;
    private static final short ROTATE_270_MIRROR = 7;
    private ImagePlus imgPlus;
    private ImageProcessor imgProc;

    FileOutputStream debugFile;
    PrintStream debugPrintStream;
    Properties properties = new Properties();
    String propertyFile = "EPICS_AD_Controller.properties";

    JCALibrary jca;
    DefaultConfiguration conf;
    Context ctxt;

    /* These are EPICS channel objects for the camera */
    Channel ch_maxSizeCamX;       //This is the maximum dimension of X
    Channel ch_maxSizeCamY;       //This is the maximum dimension of Y
    Channel ch_minCamX;           //This is the start of the CCD readout in X
    Channel ch_minCamY;           //This is the start of the CCD readout in Y
    Channel ch_minCamX_RBV;       //This is the readback value of X of the CCD readout
    Channel ch_minCamY_RBV;       //This is the readback value of Y of the CCD readout
    Channel ch_sizeCamX;          //This is the size of the CCD readout in the X dimension
    Channel ch_sizeCamY;          //This is the size of the CCD readout in the Y dimension
    Channel ch_sizeCamArrayX_RBV; //This is the size of the image array of the CCD in X
    Channel ch_sizeCamArrayY_RBV; //This is the size of the image array of the CCD in Y
    Channel ch_binCamX_RBV;       //This is the amount of binning for the CCD readout in X
    Channel ch_binCamY_RBV;       //This is the amount of binning for the CCD readout in Y
    Channel ch_reverseCamX_RBV;
    Channel ch_reverseCamY_RBV;
    /* These are EPICS channel objects for the ROI plugin */
    Channel ch_minRoiX;           //This is the start of the ROI in X
    Channel ch_minRoiY;           //This is the start of the ROI in Y
    Channel ch_minRoiX_RBV;       //This is the readback value of X in the ROI
    Channel ch_minRoiY_RBV;       //This is the readback value of Y in the ROI
    Channel ch_binRoiX;           //This is the binning of the ROI in the X dimension
    Channel ch_binRoiY;           //This is the binning of the ROI in the Y dimension
    Channel ch_sizeRoiX;          //This is the size of the ROI in the X dimension
    Channel ch_sizeRoiY;          //This is the size of the ROI in the Y dimension
    Channel ch_sizeRoiArrayX_RBV; //This is the size of the image array for the ROI in X
    Channel ch_sizeRoiArrayY_RBV; //This is the size of the image array for the ROI in Y
    Channel ch_reverseRoiX_RBV;       //This is reverse flag for the ROI in X
    Channel ch_reverseRoiY_RBV;       //This is reverse flag for the ROI in Y
    /* These are EPICS channel objects for the Transform plugin */
    Channel ch_transType;         //This represents the way the image is flipped or rotated.
    Channel ch_transArrayX_RBV;
    Channel ch_transArrayY_RBV;
    /* These are EPICS channel objects for the Overlay plugin */
    Channel ch_minOverlayX;       //This is the position of the overlay in X
    Channel ch_minOverlayY;       //This is the position of the overlay in Y
    Channel ch_sizeOverlayX;      //This is the size of the overlay in X 
    Channel ch_sizeOverlayY;      //This is the size of the overlay in Y

    JFrame frame;
    JTextField StatusText;
    JTextField cameraPrefixText;
    JTextField transformPrefixText;
    JTextField roiPrefixText;
    JTextField overlayPrefixText;
    JButton resetCameraReadoutButton;
    JButton resetROIButton;
    JComboBox outputSelectComboBox;
    String[] outputSelectChoices = {"Camera", "ROI", "Overlay"};
    JButton setControlItemButton;
    JCheckBox transformInChainCheckBox;
    JCheckBox ROIInChainCheckBox;

    boolean isDebugMessages;
    boolean isDebugFile;
    boolean isPluginRunning;
    boolean isCameraConnected;
    boolean isRoiConnected;
    boolean isTransformConnected;
    boolean isOverlayConnected;
    boolean isTransformInChain;
    boolean isROIInChain;
    String outputSelect;

    String cameraPrefix;
    String roiPrefix;
    String transformPrefix;
    String overlayPrefix;

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
            isDebugFile = true;
            isDebugMessages = true;
            isPluginRunning = true;
            isTransformInChain = false;
            Date date = new Date();
            cameraPrefix = "13SIM1:cam1:";
            transformPrefix = "13SIM1:Trans1:";
            roiPrefix = "13SIM1:ROI1:";
            overlayPrefix = "13SIM1:Over1:1:";
            isTransformInChain = false;
            isROIInChain = false;
            outputSelect = "Camera";
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
            
            startEPICSCA();
            // Connect to PVs from the camera.
            // This does not need to succeed.
            connectCameraPVs();
            // Connect to PVs from the ROI plugin.
            // This does not need to succeed.
            connectRoiPVs();
            // Connect to PVs from the transform plugin.
            // This do not need to succeed.
            connectTransformPVs();
            // Connect to PVs from the overlay plugin.
            // This do not need to succeed.
            connectOverlayPVs();

            /* This simply polls for new data and updates the image if new data
             * is found.
             */
            while (isPluginRunning) {
                synchronized (this) {
                    wait(1000);
                }
            }

            if (isDebugFile) {
                debugPrintStream.close();
                debugFile.close();
                logMessage("Closed debug file", true, true);
            }

            writeProperties();
            disconnectCameraPVs();
            disconnectRoiPVs();
            disconnectTransformPVs();
            disconnectOverlayPVs();
            closeEPICSCA();
            IJ.showStatus("Exiting Server");

        } catch (Exception e) {
            IJ.log("Got exception: " + e.getMessage());
            e.printStackTrace();
            IJ.log("Close window, and reopen, try again");

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

    public void setROI(int minX, int minY, int sizeX, int sizeY) {
        if (isRoiConnected) {
            try {
                epicsSetInt(ch_minRoiX, minX);
                epicsSetInt(ch_minRoiY, minY);
                epicsSetInt(ch_sizeRoiX, sizeX);
                epicsSetInt(ch_sizeRoiY, sizeY);
                if (isDebugMessages) {
                    logMessage("Set ROI to " + minX + "," + minY + "," + sizeX + "," + sizeY, true, true);
                }
            } catch (CAException ex) {
                IJ.log("CAException: Could not set ROI: " + ex.getMessage());
            } catch (TimeoutException ex) {
                IJ.log("TimeoutException: Could not set ROI: " + ex.getMessage());
            } catch (IllegalStateException ex) {
                IJ.log("IllegalStateException: Could not set ROI: " + ex.getMessage());
            }
        } else {
            logMessage("setROI: ROI not connected", true, true);
        }
    }

    public void resetROI() {
        int sizeX;
        int sizeY;

        try {
            // Get the maximum size of the CCD
            sizeX = epicsGetInt(ch_maxSizeCamX);
            sizeY = epicsGetInt(ch_maxSizeCamY);           
            setROI(0, 0, sizeX, sizeY);
        } catch (CAException ex) {
            IJ.log("CAException: Could not reset ROI: " + ex.getMessage());
        } catch (TimeoutException ex) {
            IJ.log("TimeoutException: Could not reset ROI: " + ex.getMessage());
        } catch (IllegalStateException ex) {
            IJ.log("IllegalStateException: Could not reset ROI: " + ex.getMessage());
        }
    }
    
    public void setOverlay(int minX, int minY, int sizeX, int sizeY) {
        if (isOverlayConnected) {
            try {
                epicsSetInt(ch_minOverlayX, minX);
                epicsSetInt(ch_minOverlayY, minY);
                epicsSetInt(ch_sizeOverlayX, sizeX);
                epicsSetInt(ch_sizeOverlayY, sizeY);
                if (isDebugMessages) {
                    logMessage("Set overlay to " + minX + "," + minY + "," + sizeX + "," + sizeY, true, true);
                }
            } catch (CAException ex) {
                IJ.log("CAException: Could not set overlay: " + ex.getMessage());
            } catch (TimeoutException ex) {
                IJ.log("TimeoutException: Could not set overlay: " + ex.getMessage());
            } catch (IllegalStateException ex) {
                IJ.log("IllegalStateException: Could not set overlay: " + ex.getMessage());
            }
        } else {
            logMessage("setOverlay: overlay not connected", true, true);
        }
    }

    public void setCameraRegion(int minX, int minY, int sizeX, int sizeY) {
        if (isCameraConnected) {
            try {
                epicsSetInt(ch_minCamX, minX);
                epicsSetInt(ch_minCamY, minY);
                epicsSetInt(ch_sizeCamX, sizeX);
                epicsSetInt(ch_sizeCamY, sizeY);
                if (isDebugMessages) {
                    logMessage("Set camera region to " + minX + "," + minY + "," + sizeX + "," + sizeY, true, true);
                }
            } catch (CAException ex) {
                IJ.log("CAException: Could not set camera region: " + ex.getMessage());
            } catch (TimeoutException ex) {
                IJ.log("TimeoutException: Could not set camera region: " + ex.getMessage());
            } catch (IllegalStateException ex) {
                IJ.log("IllegalStateException: Could not set camera region: " + ex.getMessage());
            }
        } else {
            logMessage("setCameraRegion: camera not connected", true, true);
        }
    }
      
    public void resetCameraRegion() {
        int sizeX;
        int sizeY;

        try {
            // Get the maximum size of the CCD
            sizeX = epicsGetInt(ch_maxSizeCamX);
            sizeY = epicsGetInt(ch_maxSizeCamY);           
            epicsSetInt(ch_minCamX, 0);
            epicsSetInt(ch_minCamY, 0);
            epicsSetInt(ch_sizeCamX, sizeX);
            epicsSetInt(ch_sizeCamY, sizeY);
        } catch (CAException ex) {
            IJ.log("CAException: Could not reset camera region to full: " + ex.getMessage());
        } catch (TimeoutException ex) {
            IJ.log("TimeoutException: Could not reset camera region to full: " + ex.getMessage());
        } catch (IllegalStateException ex) {
            IJ.log("IllegalStateException: Could not reset camera region to full: " + ex.getMessage());
        }
    }
    
    public Roi getROI() {
        Roi roi;

        imgPlus = WindowManager.getCurrentImage();
        if (imgPlus == null) {
            logMessage("No active image", true, true);
            return(null);
        }
        imgProc = imgPlus.getProcessor();
        roi = imgPlus.getRoi();
        if (roi == null) {
            logMessage("No ImageJ ROI found", true, true);
        }
        return roi;
    }

    public void setSelectedItem() {
        Roi roi;
        Rectangle rect;
        int minX, minY, sizeX, sizeY, binX, binY, itemp;
        CameraParameters cp=null;   
        TransformParameters tp=null;   
        ROIParameters rp=null;   

        roi = getROI();
        if (roi == null) return;
        rect = roi.getBounds();
        minX = (int) rect.getX();
        minY = (int) rect.getY();
        sizeX = (int) rect.getWidth();
        sizeY = (int) rect.getHeight();
        logMessage("Input ROI: minX="+String.valueOf(minX)+" minY="+String.valueOf(minY)+
                        " sizeX="+String.valueOf(sizeX)+" sizeY="+String.valueOf(sizeX), true, true);
        // Compute the coordinates based on transformations in the camera, transform plugin, and roi plugin
        if (isCameraConnected) {
            cp = new CameraParameters();
        }    
        if (isTransformConnected && isTransformInChain) {
            tp = new TransformParameters();
        }
        if (isRoiConnected && isROIInChain) {
            rp = new ROIParameters();
        }    

        if (outputSelect.equals("Camera") && isCameraConnected) {
            if (rp != null) {
                // The ROI is in the chain, so we need to undo its effect on image
                // We require that the ROI is showing the entire image, only need to consider bin and reverse
                if (rp.reverseX) minX = rp.imageSizeX - sizeX - minX - 1;
                if (rp.reverseY) minY = rp.imageSizeY - sizeY - minY - 1;
                minX *= rp.binX;
                minY *= rp.binY;
                sizeX *= rp.binX;
                sizeY *= rp.binY;
            }
            if (tp != null) {
                // The transform plugin is in the chain, so we need to undo its effect on image
                if (tp.reverseX) minX = tp.imageSizeX - sizeX - minX - 1;
                if (tp.reverseY) minY = tp.imageSizeY - sizeY - minY - 1;
                if (tp.swapAxes) {
                    itemp = minX;
                    minX = minY;
                    minY = itemp;
                    itemp = sizeX;
                    sizeX = sizeY;
                    sizeY = itemp;
                }
            }            
            if (cp != null) {
                // Correct for the current camera settings on the image
                if (cp.reverseX) minX = cp.imageSizeX - sizeX - minX - 1;
                if (cp.reverseY) minY = cp.imageSizeY - sizeY - minY - 1;
                minX = minX*cp.binX + cp.minX;
                minY = minY*cp.binY + cp.minY;
                sizeX *= cp.binX;
                sizeY *= cp.binY;
            }

            setCameraRegion(minX, minY, sizeX, sizeY);
        } else if (outputSelect.equals("ROI") && isRoiConnected) {
            if (rp != null) {
                // The ROI is in the chain, so we need to undo its effect on image
                if (rp.reverseX) minX = rp.imageSizeX - sizeX - minX - 1;
                if (rp.reverseY) minY = rp.imageSizeY - sizeY - minY - 1;
                minX *= rp.binX;
                minY *= rp.binY;
                sizeX *= rp.binX;
                sizeY *= rp.binY;
            }
            setROI(minX, minY, sizeX, sizeY);
        } else if (outputSelect.equals("Overlay") && isOverlayConnected) {
            // Since the overlay comes after the transform and the ROI we don't need to
            // correct for them in computing coordinates
            setOverlay(minX, minY, sizeX, sizeY);
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
            ch_binCamX_RBV = createEPICSChannel(cameraPrefix + "BinX_RBV");
            ch_binCamY_RBV = createEPICSChannel(cameraPrefix + "BinY_RBV");
            ch_reverseCamX_RBV = createEPICSChannel(cameraPrefix + "ReverseX_RBV");
            ch_reverseCamY_RBV = createEPICSChannel(cameraPrefix + "ReverseY_RBV");
            
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
            ch_transType = createEPICSChannel(transformPrefix + "Type");
            ch_transArrayX_RBV = createEPICSChannel(transformPrefix + "ArraySizeX_RBV");
            ch_transArrayY_RBV = createEPICSChannel(transformPrefix + "ArraySizeY_RBV");
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
            ch_minRoiX = createEPICSChannel(roiPrefix + "MinX");
            ch_minRoiY = createEPICSChannel(roiPrefix + "MinY");
            ch_minRoiX_RBV = createEPICSChannel(roiPrefix + "MinX_RBV");
            ch_minRoiY_RBV = createEPICSChannel(roiPrefix + "MinY_RBV");
            ch_binRoiX = createEPICSChannel(roiPrefix + "BinX");
            ch_binRoiY = createEPICSChannel(roiPrefix + "BinY");
            ch_reverseRoiX_RBV = createEPICSChannel(roiPrefix + "ReverseX_RBV");
            ch_reverseRoiY_RBV = createEPICSChannel(roiPrefix + "ReverseY_RBV");
            ch_sizeRoiX = createEPICSChannel(roiPrefix + "SizeX");
            ch_sizeRoiY = createEPICSChannel(roiPrefix + "SizeY");
            ch_sizeRoiArrayX_RBV = createEPICSChannel(roiPrefix + "ArraySizeX_RBV");
            ch_sizeRoiArrayY_RBV = createEPICSChannel(roiPrefix + "ArraySizeY_RBV");            
            ctxt.flushIO();
            checkRoiPVConnections();            
        } catch (Exception ex) {
            logMessage("CAException: Cannot connect to EPICS ROI PV:" + ex.getMessage(), true, true);
            checkRoiPVConnections();
        }  
    }

    /**
     * This method creates the PV objects for the Overlay plugin.
     */
    public void connectOverlayPVs() {
        try {
            overlayPrefix = overlayPrefixText.getText();
            logMessage("Trying to connect to EPICS PVs: " + overlayPrefix, true, true);            
            ch_minOverlayX = createEPICSChannel(overlayPrefix + "PositionX");
            ch_minOverlayY = createEPICSChannel(overlayPrefix + "PositionY");
            ch_sizeOverlayX = createEPICSChannel(overlayPrefix + "SizeX");
            ch_sizeOverlayY = createEPICSChannel(overlayPrefix + "SizeY");            
            ctxt.flushIO();
            checkOverlayPVConnections();            
        } catch (Exception ex) {
            logMessage("CAException: Cannot connect to EPICS Overlay PV:" + ex.getMessage(), true, true);
            checkOverlayPVConnections();
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
            ch_binCamX_RBV.destroy();
            ch_binCamY_RBV.destroy();
            ch_reverseCamX_RBV.destroy();
            ch_reverseCamY_RBV.destroy();
            isCameraConnected = false;            
            logMessage("Disconnected from EPICS camera PVs OK", true, true);
        } catch (CAException ex) {
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
            ch_transType.destroy();
            ch_transArrayX_RBV.destroy();
            ch_transArrayY_RBV.destroy();
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
            ch_binRoiX.destroy();
            ch_binRoiY.destroy();
            ch_reverseRoiX_RBV.destroy();
            ch_reverseRoiY_RBV.destroy();
            ch_sizeRoiX.destroy();
            ch_sizeRoiY.destroy();
            ch_sizeRoiArrayX_RBV.destroy();
            ch_sizeRoiArrayY_RBV.destroy();
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
     * This method destroys the PV objects for the Overlay plugin.
     */
    public void disconnectOverlayPVs() {
        try {
            ch_minOverlayX.destroy();
            ch_minOverlayY.destroy();
            ch_sizeOverlayX.destroy();
            ch_sizeOverlayY.destroy();
            isOverlayConnected = false;            
            logMessage("Disconnected from EPICS Overlay PVs OK", true, true);
        } catch (CAException ex) {
            logMessage("CAException: Cannot disconnect from EPICS Overlay PV:" + ex.getMessage(), true, true);
        } catch (IllegalStateException ex) {
            logMessage("IllegalStateException: Cannot disconnect from EPICS Overlay PV:" + ex.getMessage(), true, true);
        } catch (NullPointerException ex) {
            logMessage("NullPointerException: Cannot disconnect from EPICS Overlay PV:" + ex.getMessage(), true, true);
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
     * This method checks that the PV objects for the camera both exist
     * and are connected to the underlying PVs.
     */
    public void checkCameraPVConnections() {
        boolean cameraConnected;
        cameraConnected = 
              (ch_minCamX != null && ch_minCamX.getConnectionState() == Channel.ConnectionState.CONNECTED
            && ch_minCamY != null && ch_minCamY.getConnectionState() == Channel.ConnectionState.CONNECTED
            && ch_minCamX_RBV != null && ch_minCamX_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
            && ch_minCamY_RBV != null && ch_minCamY_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
            && ch_sizeCamX != null && ch_sizeCamX.getConnectionState() == Channel.ConnectionState.CONNECTED
            && ch_sizeCamY != null && ch_sizeCamY.getConnectionState() == Channel.ConnectionState.CONNECTED
            && ch_maxSizeCamX != null && ch_maxSizeCamX.getConnectionState() == Channel.ConnectionState.CONNECTED
            && ch_maxSizeCamY != null && ch_maxSizeCamY.getConnectionState() == Channel.ConnectionState.CONNECTED
            && ch_binCamX_RBV != null && ch_binCamX_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
            && ch_binCamY_RBV != null && ch_binCamY_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
            && ch_reverseCamX_RBV != null && ch_reverseCamX_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
            && ch_reverseCamY_RBV != null && ch_reverseCamY_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
            && ch_sizeCamArrayX_RBV != null && ch_sizeCamArrayX_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
            && ch_sizeCamArrayY_RBV != null && ch_sizeCamArrayY_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED);

        if (cameraConnected & !isCameraConnected) {
            isCameraConnected = true;
            logMessage("Connection to EPICS camera PVs OK", true, true);
            cameraPrefixText.setBackground(Color.green);               
        }
        if (!cameraConnected) {
            isCameraConnected = false;
            logMessage("Cannot connect to EPICS camera PVs", true, true);
            cameraPrefixText.setBackground(Color.red);
        }
    }
    
    /**
     * This method checks that the PV objects for the transform plugin both exist
     * and are connected to the underlying PVs.
     */
    public void checkTransformPVConnections() {
        boolean transformConnected;
        
        try {
            transformConnected = (ch_transType != null && ch_transType.getConnectionState() == Channel.ConnectionState.CONNECTED
                               && ch_transArrayX_RBV != null && ch_transArrayX_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
                               && ch_transArrayY_RBV != null && ch_transArrayY_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED);
            if (transformConnected & !isTransformConnected) {
                isTransformConnected = true;
                logMessage("Connection to EPICS transform PVs OK", true, true);
                transformPrefixText.setBackground(Color.green);
                transformInChainCheckBox.setEnabled(true);
            }
            if (!transformConnected) {
                isTransformConnected = false;
                logMessage("Cannot connect to EPICS transform PVs", true, true);
                transformPrefixText.setBackground(Color.red);
                transformInChainCheckBox.setSelected(false);
                transformInChainCheckBox.setEnabled(false);
                isTransformInChain = false;
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

        roiConnected = 
                  (ch_minRoiX != null && ch_minRoiX.getConnectionState() == Channel.ConnectionState.CONNECTED
                && ch_minRoiY != null && ch_minRoiY.getConnectionState() == Channel.ConnectionState.CONNECTED
                && ch_minRoiX_RBV != null && ch_minRoiX_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
                && ch_minRoiY_RBV != null && ch_minRoiY_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
                && ch_binRoiX != null && ch_binRoiX.getConnectionState() == Channel.ConnectionState.CONNECTED
                && ch_binRoiY != null && ch_binRoiY.getConnectionState() == Channel.ConnectionState.CONNECTED
                && ch_reverseRoiX_RBV != null && ch_reverseRoiX_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
                && ch_reverseRoiY_RBV != null && ch_reverseRoiY_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
                && ch_sizeRoiX != null && ch_sizeRoiX.getConnectionState() == Channel.ConnectionState.CONNECTED
                && ch_sizeRoiY != null && ch_sizeRoiY.getConnectionState() == Channel.ConnectionState.CONNECTED
                && ch_sizeRoiArrayX_RBV != null && ch_sizeRoiArrayX_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED
                && ch_sizeRoiArrayY_RBV != null && ch_sizeRoiArrayY_RBV.getConnectionState() == Channel.ConnectionState.CONNECTED);
        if (roiConnected & !isRoiConnected) {
            isRoiConnected = true;
            logMessage("Connection to EPICS ROI PVs OK", true, true);
            roiPrefixText.setBackground(Color.green);
        }
        if (!roiConnected) {
            isRoiConnected = false;
            logMessage("Cannot connect to EPICS ROI PVs", true, true);
            roiPrefixText.setBackground(Color.red);
        }            
    }

     /**
     * This method checks that the PV objects for the Overlay plugin both exist
     * and are connected to the underlying PVs.
     */
    public void checkOverlayPVConnections() {
        boolean overlayConnected;

        try {
            overlayConnected = 
                      (ch_minOverlayX != null && ch_minOverlayX.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_minOverlayY != null && ch_minOverlayY.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_sizeOverlayX != null && ch_sizeOverlayX.getConnectionState() == Channel.ConnectionState.CONNECTED
                    && ch_sizeOverlayY != null && ch_sizeOverlayY.getConnectionState() == Channel.ConnectionState.CONNECTED);
            if (overlayConnected & !isOverlayConnected) {
                epicsGetInt(ch_minOverlayX);
                epicsGetInt(ch_minOverlayY);
                isOverlayConnected = true;
                logMessage("Connection to EPICS Overlay PVs OK", true, true);
                overlayPrefixText.setBackground(Color.green);
            }
            if (!overlayConnected) {
                isOverlayConnected = false;
                logMessage("Cannot connect to EPICS Overlay PVs", true, true);
                overlayPrefixText.setBackground(Color.red);
            }            
        } catch (IllegalStateException ex) {
            IJ.log("checkOverlayPVConnections: got exception= " + ex.getMessage());
            if (isDebugFile) {
                ex.printStackTrace(debugPrintStream);
            }
        } catch (TimeoutException ex) {
           IJ.log("checkOverlayPVConnections: got exception= " + ex.getMessage());
            if (isDebugFile) {
                ex.printStackTrace(debugPrintStream);
            }
        } catch (CAException ex) {
            IJ.log("checkOverlayPVConnections: got exception= " + ex.getMessage());
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
     * This class is used to determine the effect of the transform plugin
     */
    private class TransformParameters {
        private int imageSizeX;
        private int imageSizeY;
        private boolean swapAxes;
        private boolean reverseX;
        private boolean reverseY;
        public TransformParameters() {
            this.read();
        }
  
        public void read() {
            int transformType = NONE;
            try {
                transformType = epicsGetEnum(ch_transType);
                imageSizeX = epicsGetInt(ch_transArrayX_RBV);
                imageSizeY = epicsGetInt(ch_transArrayY_RBV);
            } catch (CAException ex) {
                IJ.log("TransformParameters CAException: Could not get parameters from " + transformPrefix + ": " + ex.getMessage());
            } catch (TimeoutException ex) {
                IJ.log("TransformParameters TimeoutException: Could not get parameters from " + transformPrefix + ": " + ex.getMessage());
            } catch (IllegalStateException ex) {
                IJ.log("TransformParameters IllegalStateException: Could not get parameters from " + transformPrefix + ": " + ex.getMessage());
            }

            switch (transformType) {                    
                case NONE:
                    swapAxes = false;
                    reverseX = false;
                    reverseY = false;
                    break;
                case ROTATE_90:
                    swapAxes = true;
                    reverseX = true;
                    reverseY = false;
                    break;
                case ROTATE_270:
                    swapAxes = true;
                    reverseX = false;
                    reverseY = true;
                    break;
                case ROTATE_180:
                    swapAxes = false;
                    reverseX = true;
                    reverseY = true;
                    break;
                case ROTATE_90_MIRROR:
                    swapAxes = true;
                    reverseX = false;
                    reverseY = false;
                    break;
               case ROTATE_270_MIRROR:
                    swapAxes = true;
                    reverseX = true;
                    reverseY = true;
                    break;
               case MIRROR:
                    swapAxes = false;
                    reverseX = true;
                    reverseY = false;
                    break;
               case ROTATE_180_MIRROR:
                    swapAxes = false;
                    reverseX = false;
                    reverseY = true;
                    break;
                default:
                    logMessage("TransformParameters: found no matching transform type", true, true);
                    break;
            }
            if (isDebugMessages)
                logMessage("TransformParameters swapAxes="+String.valueOf(swapAxes)+
                                              " reverseX="+String.valueOf(reverseX)+
                                              " reverseY="+String.valueOf(reverseY), true, true);
        }
    }

    /**
     * This class is used to determine the effect of the ROI plugin
     */
    private class ROIParameters {
        public int minX;
        public int minY;
        public int binX;
        public int binY;
        public boolean reverseX;
        public boolean reverseY;
        public int imageSizeX;
        public int imageSizeY;
        public ROIParameters() {
            this.read();
        }
        public void read() {
            try {
                minX = epicsGetInt(ch_minRoiX_RBV);
                minY = epicsGetInt(ch_minRoiY_RBV);
                binX = epicsGetInt(ch_binRoiX);
                binY = epicsGetInt(ch_binRoiY);
                reverseX = epicsGetInt(ch_reverseRoiX_RBV) != 0;
                reverseY = epicsGetInt(ch_reverseRoiY_RBV) != 0;
                imageSizeX = epicsGetInt(ch_sizeRoiArrayX_RBV);
                imageSizeY = epicsGetInt(ch_sizeRoiArrayY_RBV);
            } catch (CAException ex) {
                IJ.log("ROIParameters CAException: Could not get parameters from " + roiPrefix + ": " + ex.getMessage());
            } catch (TimeoutException ex) {
                IJ.log("ROIParameters TimeoutException: Could not get parameters from " + roiPrefix + ": " + ex.getMessage());
            } catch (IllegalStateException ex) {
                IJ.log("ROIParameters IllegalStateException: Could not get parameters from " + roiPrefix + ": " + ex.getMessage());
            }
        }
    }

    private class CameraParameters {
        public int minX;
        public int minY;
        public int imageSizeX;
        public int imageSizeY;
        public int binX;
        public int binY;
        public boolean reverseX;
        public boolean reverseY;
        public CameraParameters() {
            this.read();
        }
        public void read() {
            try {
                minX = epicsGetInt(ch_minCamX_RBV);
                minY = epicsGetInt(ch_minCamY_RBV);
                binX = epicsGetInt(ch_binCamX_RBV);
                binY = epicsGetInt(ch_binCamY_RBV);
                imageSizeX = epicsGetInt(ch_sizeCamArrayX_RBV);
                imageSizeY = epicsGetInt(ch_sizeCamArrayY_RBV);
                reverseX = epicsGetInt(ch_reverseCamX_RBV) != 0;
                reverseY = epicsGetInt(ch_reverseCamY_RBV) != 0;
            } catch (CAException ex) {
                IJ.log("ROIParameters CAException: Could not get parameters from " + cameraPrefix + ": " + ex.getMessage());
            } catch (TimeoutException ex) {
                IJ.log("ROIParameters TimeoutException: Could not get parameters from " + cameraPrefix + ": " + ex.getMessage());
            } catch (IllegalStateException ex) {
                IJ.log("ROIParameters IllegalStateException: Could not get parameters from " + cameraPrefix + ": " + ex.getMessage());
            }
        }
    }

    /**
     * Create the GUI and show it. For thread safety, this method should be
     * invoked from the event-dispatching thread.
     */
    public void createAndShowGUI() {
        //Create and set up the window.
        StatusText = new JTextField(50);
        StatusText.setEditable(false);
        
        cameraPrefixText = new JTextField(cameraPrefix, 15);
        transformPrefixText = new JTextField(transformPrefix, 15);
        roiPrefixText = new JTextField(roiPrefix, 15);
        overlayPrefixText = new JTextField(overlayPrefix, 15);
        resetCameraReadoutButton = new JButton("Reset camera region");
        resetROIButton = new JButton("Reset ROI");
        transformInChainCheckBox = new JCheckBox("Transform Plugin In Chain", isTransformInChain);
        ROIInChainCheckBox = new JCheckBox("ROI Plugin In Chain", isROIInChain);
        outputSelectComboBox = new JComboBox(outputSelectChoices);
        outputSelectComboBox.setSelectedItem(outputSelect);
        setControlItemButton = new JButton("Set");
        
        frame = new JFrame("Image J EPICS_AD_Controller Plugin");
        JPanel panel = new JPanel(new BorderLayout());
        panel.setLayout(new GridBagLayout());
        panel.setBorder(new EmptyBorder(new Insets(5, 5, 5, 5)));
        frame.getContentPane().add(BorderLayout.CENTER, panel);
        GridBagConstraints c = new GridBagConstraints();
        // Add extra space around each component to avoid clutter
        c.insets = new Insets(2, 2, 2, 2);

        // Top row
        // These widgets should be centered
        c.anchor = GridBagConstraints.CENTER;
        c.gridx = 0;
        c.gridy = 0;
        panel.add(new JLabel("Camera PV Prefix"), c);
        c.gridx++;
        panel.add(new JLabel("Transform PV Prefix"), c);
        c.gridx++;
        panel.add(new JLabel("ROI PV Prefix"), c);
        c.gridx++;
        panel.add(new JLabel("Overlay PV Prefix"), c);
        c.gridx++;
        panel.add(new JLabel("Output PVs"), c);

        // Second row
        c.anchor = GridBagConstraints.CENTER;
        c.gridy = 1;
        c.gridx = 0;
        panel.add(cameraPrefixText, c);
        c.gridx++;
        panel.add(transformPrefixText, c);
        c.gridx++;
        panel.add(roiPrefixText, c);
        c.gridx++;
        panel.add(overlayPrefixText, c);
        c.gridx++;
        c.anchor = GridBagConstraints.WEST;
        panel.add(outputSelectComboBox, c);
        c.gridx++;
        c.anchor = GridBagConstraints.WEST;
        panel.add(setControlItemButton, c);

        // Third row
        c.anchor = GridBagConstraints.CENTER;
        c.gridy = 2;
        c.gridx = 0;
        panel.add(resetCameraReadoutButton, c);
        c.gridx++;
        panel.add(transformInChainCheckBox, c);
        c.gridx++;
        panel.add(resetROIButton, c);
        c.gridx++;
        panel.add(ROIInChainCheckBox, c);
        c.gridx++;

        // Fifth row
        c.gridy = 4;
        c.gridx = 0;
        c.anchor = GridBagConstraints.EAST;
        panel.add(new JLabel("Status: "), c);
        c.gridx++;
        c.gridwidth = 7;
        c.anchor = GridBagConstraints.WEST;
        panel.add(StatusText, c);

        //Display the window.
        frame.pack();
        frame.addWindowListener(new FrameExitListener());
        frame.setVisible(true);

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
        
        overlayPrefixText.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent event) {
                if (isDebugMessages)
                    IJ.log("Overlay prefix changed");
                disconnectOverlayPVs();
                connectOverlayPVs();
            }
        });

        resetCameraReadoutButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent event) {
                resetCameraRegion();
            }
        });

        resetROIButton.addActionListener(new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent event) {
                resetROI();
            }
        });

        transformInChainCheckBox.addItemListener(new ItemListener () {
            @Override
            public void itemStateChanged(ItemEvent e) {
                isTransformInChain = (e.getStateChange() == ItemEvent.SELECTED);
            }
        });

        ROIInChainCheckBox.addItemListener(new ItemListener () {
            @Override
            public void itemStateChanged(ItemEvent e) {
                isROIInChain = (e.getStateChange() == ItemEvent.SELECTED);
            }
        });

        outputSelectComboBox.addActionListener(new ActionListener () {
            @Override
            public void actionPerformed(ActionEvent e) {
                outputSelect = String.valueOf(outputSelectComboBox.getSelectedItem());
            }
        });                

        setControlItemButton.addActionListener(new ActionListener () {
            @Override
            public void actionPerformed(ActionEvent event) {
                setSelectedItem();
            }
        });

    }

    public class FrameExitListener extends WindowAdapter {

        @Override
        public void windowClosing(WindowEvent event) {
            isPluginRunning = false;
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
            // StatusText won't exist until the GUI is created, so early logMessage calls won't have it
            if (StatusText != null) StatusText.setText(completeMessage);
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
            temp = properties.getProperty("overlayPrefix");
            if (temp != null) {
                overlayPrefix = temp;
            }
            temp = properties.getProperty("isTransformInChain");
            if (temp != null) {
                isTransformInChain = Boolean.parseBoolean(temp);
            }
            temp = properties.getProperty("isROIInChain");
            if (temp != null) {
                isROIInChain = Boolean.parseBoolean(temp);
            }
            temp = properties.getProperty("outputSelect");
            if (temp != null) {
                outputSelect = temp;
            }
            IJ.log("Read properties file: " + path );
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
            properties.setProperty("cameraPrefix", cameraPrefix);
            properties.setProperty("transformPrefix", transformPrefix);
            properties.setProperty("roiPrefix", roiPrefix);
            properties.setProperty("overlayPrefix", overlayPrefix);
            properties.setProperty("isTransformInChain", String.valueOf(isTransformInChain));
            properties.setProperty("isROIInChain", String.valueOf(isROIInChain));
            properties.setProperty("outputSelect", outputSelect);
            FileOutputStream file = new FileOutputStream(path);
            properties.store(file, "EPICS_AD_Viewer Properties");
            file.close();
            IJ.log("Wrote properties file: " + path);
        } catch (IOException ex) {
            IJ.log("writeProperties:exception: " + ex.getMessage());
        }
    }
}
