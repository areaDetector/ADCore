package gov.anl.aps.synApps.areaDetector;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.URL;

import org.eclipse.core.resources.IContainer;
import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IFolder;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IWorkspaceRoot;
import org.eclipse.core.resources.ResourcesPlugin;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.FileLocator;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.NullProgressMonitor;
import org.eclipse.core.runtime.Path;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.IWorkbenchWindowActionDelegate;

public class InstallAreaOPITemplates extends Action implements
		IWorkbenchWindowActionDelegate {

	public InstallAreaOPITemplates() {
		// TODO Auto-generated constructor stub
	}

	public InstallAreaOPITemplates(String text) {
		super(text);
		// TODO Auto-generated constructor stub
	}

	public InstallAreaOPITemplates(String text, ImageDescriptor image) {
		super(text, image);
		// TODO Auto-generated constructor stub
	}

	public InstallAreaOPITemplates(String text, int style) {
		super(text, style);
		// TODO Auto-generated constructor stub
	}

	@Override
	public void dispose() {
		// TODO Auto-generated method stub

	}

	@Override
	public void init(IWorkbenchWindow window) {
		// TODO Auto-generated method stub

	}

	@Override
	public void run(IAction action) {
		// TODO Auto-generated method stub
		System.out.println("Running action");
		final IWorkspaceRoot root = ResourcesPlugin.getWorkspace().getRoot();

		Job job = new Job("Install AreaDetector templates") {
			protected IStatus run(IProgressMonitor monitor) {
				try {
					IProject project = root.getProject("gov.anl.aps.synApps.areaDetector");
					if (!project.exists()) {
						// create root project for synApps if it does not exist
						// in our workspace
						project.create(new NullProgressMonitor());
					}
					project.open(new NullProgressMonitor());
					String folderToCopy = new String("templates");
					URL url = FileLocator.find(Activator.getDefault()
							.getBundle(), new Path(folderToCopy),
							null);
					try {
						File directory = new File(FileLocator.toFileURL(url)
								.getPath());
						if (directory.isDirectory()) {
							File[] files = directory.listFiles();
							IFolder folder = project.getFolder(folderToCopy);
							if (!folder.exists()){
								folder.create(true, true, monitor);
							}
							monitor
									.beginTask("Copying Templates",
											count(files));
							copy(files, folder, monitor);
						}
					} catch (IOException ex) {

					}
				}

				catch (CoreException ex) {
					ex.printStackTrace();
				}
			return Status.OK_STATUS;
			}
		};
		job.schedule();
	}


	private void copy (File[] files, IContainer container, IProgressMonitor monitor){
		try {
			for (File file: files) {
				monitor.subTask("Copying" + file.getName());
				if (file.isDirectory()) {
					IFolder folder = container.getFolder(new Path(file.getName()));
					if (!folder.exists()){
						folder.create(true, true, null);
					}
					copy(file.listFiles(), folder, monitor);
				}
				else {
					IFile pFile = container.getFile(new Path(file.getName()));
					if (!pFile.exists()){
						pFile.create(new FileInputStream(file), true, new NullProgressMonitor());
					}
					monitor.internalWorked(1);
				}
			}
		}
		catch (Exception ex){
			
		}
	}
	private int count (File[] files){
		int result = 0;
		for (File file : files){
			if (file.isDirectory()){
				result += count(file.listFiles());
			}
			else {
				result++;
			}
		}
		return result;
	}
	
	@Override
	public void selectionChanged(IAction action, ISelection selection) {
		// TODO Auto-generated method stub

	}

}
