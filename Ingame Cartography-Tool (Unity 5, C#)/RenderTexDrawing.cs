using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;

public class RenderTexDrawing : MonoBehaviour
{
    /**CAPTUREMODE RENDERTEXTURE ONLY OR TEXTUREMODE**/
    private bool rtModeON = true;

    #region cursor variables
    [SerializeField]
    private Transform _cursor;

    Vector3 speed;
    float acceleration = 25f;
    Vector3 offsetCursor;

    #endregion

    #region capture variables

    [SerializeField]
    private Camera _renderCam;
    private RenderTexture captureRenderTexture;
    private RenderTexture[] renderTexturePool;
    private int poolIndex = 0;
    private RenderTexture lastCaptureTexture;
    private Texture2D captureTexture;
    private Camera captureCamera;
    [SerializeField]
    private GameObject captureTarget;
    private MeshRenderer captureRenderer;

    private int captureResolution = 1024;
    private int captureAASetting = 1;
    private int captureWidth = 1920;
    private int captureHeight = 1080;

    #endregion

    #region stamping variables
    [SerializeField]
    private GameObject _stampPrefab;
    #endregion

    #region splineTool variables

    [SerializeField]
    private GameObject _splineRenderTarget;
    [SerializeField]
    private Sprite circleCursor;
    #endregion

    #region eraserTool variables
    [SerializeField]
    private Sprite eraserSprite;
    [SerializeField]
    private Sprite eraserCursor;
    #endregion

    #region meterTool variables
    [SerializeField]
    private Sprite meterCursor;
    #endregion

    #region General Tools
    [SerializeField]
    private MeshLine _line; //renderer of the red line used to measure distances
    private ToolMenu _toolMenu; //radial tool menu

    //different Tools to choose from in the menu
    private ITool activeTool;
    private SplineTool splineTool;
    private StampTool stampTool;
    private MeterTool meterTool;
    private EraserTool eraserTool;


    bool undid = false;
    private int backupCount = 1; //number of steps you can undo increasing the number comes at a heavy performance cost
    private int currentBackup = -1; //currently active backup
    private Color32[][] backups;
    #endregion

    [SerializeField]
    private RectTransform UIImageTransform;

    private bool captureFrame = true;

    #region Standard Methods (Start, Update, etc)
    // Use this for initialization
    void Start()
    {
        Debug.Log(QualitySettings.GetQualityLevel().ToString());
        if (QualitySettings.GetQualityLevel() >= 4)
        {
            Debug.Log("CaptureRes: 2048");
            captureResolution = 1024;
        }
        if (QualitySettings.GetQualityLevel() == 5)
        {
            Debug.Log("CaptureAA: 2");
            captureAASetting = 2;
        }
        else if (QualitySettings.GetQualityLevel() > 5)
        {
            Debug.Log("CaptureAA: 4");
            captureAASetting = 1;
        }

        _toolMenu = GetComponent<ToolMenu>();
        captureCamera = transform.Find("CaptureCamera").GetComponent<Camera>();
        captureTexture = new Texture2D(captureResolution, captureResolution, TextureFormat.ARGB32, true);
        captureRenderer = captureTarget.GetComponent<MeshRenderer>();
        renderTexturePool = new RenderTexture[2];
        for (int i = 0; i < 2; ++i)
        {
            renderTexturePool[i] = new RenderTexture(captureResolution, captureResolution, 0, RenderTextureFormat.ARGB32, RenderTextureReadWrite.Default);
            renderTexturePool[i].antiAliasing = captureAASetting;
            renderTexturePool[i].Create();
        }


        splineTool = new SplineTool(this, _splineRenderTarget, _line, circleCursor);
        splineTool.SetSplineJaggedness(false);
        stampTool = new StampTool(this, _stampPrefab);
        meterTool = new MeterTool(this, _line, meterCursor);
        eraserTool = new EraserTool(this, eraserSprite, eraserCursor);


        activeTool = splineTool;
        activeTool.Activate();

        float mapWHratio = 16.0f / 9.0f;
        int screenWidth = Screen.width;
        screenWidth -= screenWidth / 20;
        screenWidth = (int) Mathf.Min(captureResolution * 1.25f, screenWidth);
        int height = (int)(screenWidth / mapWHratio);
        UIImageTransform.sizeDelta = new Vector2(screenWidth, height);

        backups = new Color32[backupCount][];
        for(int i = 0; i< backupCount; ++i)
        {
            backups[i] = new Color32[captureResolution * captureResolution];
        }

    }
	
	// Update is called once per frame
	void Update ()
    {
        if (captureFrame)
        {
            captureFrame = false;
            //Debug.Log(Time.deltaTime);
        }
        //pressing Y opens the Toolmenu, this blocks all other input
        if (Input.GetButtonDown("Y"))
        {
            _toolMenu.Activate();
            activeTool.Deactivate();
        }

        if (Input.GetButtonUp("Y"))
        {
            int toolIndex = _toolMenu.Deactivate();
            switch(toolIndex)
            {
                case 0:
                    activeTool = splineTool;
                    break;
                case 1:
                    activeTool = stampTool;
                    break;
                case 2:
                    activeTool = meterTool;
                    break;
                case 3:
                    activeTool = eraserTool;
                    break;
            }
            activeTool.Activate();
        }

        //If the Toolmenu is not open, process other inputs
        if(!Input.GetButton("Y"))
        { 
            //The Left-Stick movement is used for CursorMovement
            float h = Input.GetAxis("Horizontal");
            float v = Input.GetAxis("Vertical");
            UpdateCursorPosition(h, v);

            offsetCursor = new Vector3(_cursor.position.x, _cursor.position.y, _cursor.position.z + 0.1f);

            //All a,b,x button inputs are delegated to the active tool
            activeTool.Update(offsetCursor, _cursor.localRotation, _cursor.localScale.x);
            if(Input.GetButton("A"))
                activeTool.ButtonA();
            if (Input.GetButtonDown("A"))
                activeTool.ButtonADown();
            if (Input.GetButtonUp("A"))
                activeTool.ButtonAUp();
            if (Input.GetButton("B"))
                activeTool.ButtonB();
            if (Input.GetButtonDown("B"))
                activeTool.ButtonBDown();
            if (Input.GetButtonUp("B"))
                activeTool.ButtonBUp();

            if (Input.GetButton("X"))
                activeTool.ButtonX();
            if (Input.GetButtonDown("X"))
                activeTool.ButtonXDown();
            if (Input.GetButtonUp("X"))
                activeTool.ButtonXUp();

            if(Input.GetButtonDown("LB"))
            {
                Undo();
            }

            if(Input.GetButtonDown("RB"))
            {
                Redo();
            }


            //The Right-Stick movement is used for CursorRotation(xAxis) and -Scaling(yAxis) MOVETO:StampTool
            float rX = Input.GetAxis("RightStickX");
            float rY = Input.GetAxis("RightStickY");
            if (Mathf.Abs(rX) > 0.3f || Mathf.Abs(rY) > 0.3f)
                activeTool.RightStick(rX, rY);
            
        }

        speed *= 0.75f;//friction
    }
    #endregion

    #region CaptureMethods

    public void Capture()
    {
        if(rtModeON)
        {
            PureCapture();
        }
        else
        {
            TextureCapture();
        }
    }

    public void EraserCapture(bool firstFrame)
    {
        if(rtModeON)
        {
            PureEraserCapture(firstFrame);
        }
        else
        {
            TextureCapture();
        }
    }


    //This Version of the Capture Method uses Textures to permanently rasterize the captured pixels.
    //This leads to severe framerate drops when capturing quickly in succession (like when erasing)
    private void TextureCapture()
    {
        Debug.Log("Textured Capture");

        //initialize camera and texture
        captureCamera.enabled = true;       //activate the cameracomponent
        RenderTexture original = RenderTexture.active;  //save out the drawingtexture to reenable it later
        captureRenderTexture = RenderTexture.GetTemporary(captureResolution, captureResolution);   //get a temporary RenderTexture
        
        //activate rendertexture and link it to the camera
        RenderTexture.active = captureRenderTexture; //activate the captureRenderTexture
        captureCamera.targetTexture = captureRenderTexture; //link the captureRenderTexture to the camera

        //Snapshot with the CaptureCamera to update the drawing
        captureCamera.Render();
        
        //Make a Backup and transfer the Snapshot to a normal Texture
        NewBackup();
           
        captureTexture.ReadPixels(new Rect(0, 0, captureResolution, captureResolution), 0, 0); //Read out the renderTexture to rasterize it
        captureTexture.Apply(false);              //Apply the changes to the captureTexture


        captureTexture = new Texture2D(captureResolution, captureResolution, TextureFormat.ARGB32, true); //Set the Texture again (only needed the first time to render it properly
        captureFrame = true; //set Captureframe to true for debug

        //remove the temporary stuff
        RenderTexture.ReleaseTemporary(captureRenderTexture);
        captureCamera.targetTexture = null;
        captureCamera.enabled = false;
        RenderTexture.active = original;
    }

    //This Version of the Capture Method does not use a Texture
    //it does not suffer from the extreme framerate drops of the texturemethod
    private void PureCapture()
    {
        Debug.Log("Pure Capture");

        //initialize camera and texture
        captureCamera.enabled = true;       //activate the cameracomponent
        RenderTexture original = RenderTexture.active;//save out the drawingtexture to reenable it later
        if(!undid)
        {
            if (lastCaptureTexture != null)
                //RenderTexture.ReleaseTemporary(lastCaptureTexture);
            lastCaptureTexture = captureRenderTexture;
        }

        //captureRenderTexture = RenderTexture.GetTemporary(captureResolution, captureResolution, 0, RenderTextureFormat.Default, RenderTextureReadWrite.Default, captureAASetting );
        captureRenderTexture = renderTexturePool[poolIndex];
        poolIndex = (poolIndex + 1) % 2;
        //activate rendertexture and link it to the camera
        captureCamera.targetTexture = captureRenderTexture; //link the captureRenderTexture to the camera
        RenderTexture.active = captureCamera.targetTexture; //activate the captureRenderTexture

        //Snapshot with the CaptureCamera to update the drawing
        captureCamera.Render();

        captureRenderer.materials[0].mainTexture = captureRenderTexture; //Set the Texture again (only needed the first time to render it properly
        captureFrame = true; //set Captureframe to true for debug

        //remove the temporary stuff
        undid = false;
        captureCamera.targetTexture = null;
        captureCamera.enabled = false;
        RenderTexture.active = original;
    }

    //This variant of the Pure Capture Method also takes into consideration that erasing is a continuos process and only saves a backup when the erase button is pressed down
    private void PureEraserCapture(bool firstFrame)
    {
        if(firstFrame)
            Debug.Log("Pure Eraser Capture");
        RenderTexture rt = null;
        //initialize camera and texture
        captureCamera.enabled = true;       //activate the cameracomponent
        RenderTexture original = RenderTexture.active;//save out the drawingtexture to reenable it later
        if (!undid && firstFrame)
        {
            if (lastCaptureTexture != null)
                //RenderTexture.ReleaseTemporary(lastCaptureTexture);
            lastCaptureTexture = captureRenderTexture;
        }
        else
        {
            rt = captureRenderTexture;
        }

        //captureRenderTexture = RenderTexture.GetTemporary(captureResolution, captureResolution, 0, RenderTextureFormat.Default, RenderTextureReadWrite.Default, captureAASetting);
        captureRenderTexture = renderTexturePool[poolIndex];
        poolIndex = (poolIndex + 1) % 2;
        //activate rendertexture and link it to the camera
        captureCamera.targetTexture = captureRenderTexture; //link the captureRenderTexture to the camera
        RenderTexture.active = captureCamera.targetTexture; //activate the captureRenderTexture

        //Snapshot with the CaptureCamera to update the drawing
        captureCamera.Render();

        captureRenderer.materials[0].mainTexture = captureRenderTexture; //Set the Texture again (only needed the first time to render it properly
        captureFrame = true; //set Captureframe to true for debug

        //remove the temporary stuff
        if(rt != null)
            //RenderTexture.ReleaseTemporary(rt);
        undid = false;
        captureCamera.targetTexture = null;
        captureCamera.enabled = false;
        RenderTexture.active = original;
    }

    #endregion

    #region Undo/Backup
    private void Undo()
    {
        if(rtModeON)
        {
            RTUndo();
        }
        else
        {
            TUndo();
        }
    }

    private void Redo()
    {
        if(rtModeON)
        {
            RTRedo();
        }
    }
    
    //Move the backups over, copy the new backup in and set it as the current one
    void NewBackup()
    {
        ShiftBackups();
        CopyArray(captureTexture.GetPixels32(), backups[0]);
        currentBackup = -1;
    }

    void TUndo()
    {
        if (currentBackup != backupCount - 1)
        {
            currentBackup++;
            Debug.Log("undo and load backup no: " + currentBackup);
            ReadBackup(currentBackup);
        }
    }

    void ReadBackup(int i)
    {
        CopyToTexture(backups[i], captureTexture);
    }


    //moves all backups one slot to the back to free up slot 0
    void ShiftBackups()
    {
        //Move every backup to its next slot
        for(int i = backupCount-1; i > 0; --i)
        {
            CopyArray(backups[i - 1], backups[i]);
        }
    }

    void CopyToTexture(Color32[] source, Texture2D target)
    {
        if (source.Length == target.width * target.height)
        {
            target.SetPixels32(source);
            target.Apply();
        }
    }

    void CopyArray(Color32[] source, Color32[] target)
    {
        for (int i = 0; i < target.Length; ++i)
        {
            target[i] = source[i];
        }
    }


    /**RENDERTEXTURE UNDO**********/

    private void RTUndo()
    {
        captureTarget.GetComponent<MeshRenderer>().materials[0].mainTexture = lastCaptureTexture;
        undid = true;
    }

    private void RTRedo()
    {
        if(undid)
        {
            captureTarget.GetComponent<MeshRenderer>().materials[0].mainTexture = captureRenderTexture;
            undid = false;
        }
    }

    #endregion


    #region CursorMethods
    //move the cursor
    void UpdateCursorPosition(float h, float v)
    {
        Vector3 axisVector = h * transform.right + v * transform.up;
        axisVector = axisVector.sqrMagnitude >= 0.03 ? axisVector : new Vector3();

        /****move Reticle based on acceleration and left stick vector****/
        speed += acceleration * axisVector * Time.deltaTime; //make velocityvector
        _cursor.position += speed * Time.deltaTime; //make movementvector
        _cursor.localPosition = new Vector3(Mathf.Clamp(_cursor.localPosition.x, -6.8f, 6.8f), Mathf.Clamp(_cursor.localPosition.y, -3, 4f), _cursor.localPosition.z);
    }

    //Rotate(xAxis) and Scale(yAxis) the cursor
    public void RotateAndScaleCursor(float rX, float rY, float rotationSpeed, float scaleSpeed)
    {

        if (Mathf.Abs(rX) > 0.5f && Mathf.Abs(rY) < 0.5f)
        {
            _cursor.Rotate(new Vector3(0f, 0f, Mathf.Sign(rX) * rotationSpeed * Time.deltaTime));
        }
        else if (Mathf.Abs(rX) < 0.5f && Mathf.Abs(rY) > 0.5f)
        {
            _cursor.localScale += -Mathf.Sign(rY) * new Vector3(scaleSpeed, scaleSpeed, scaleSpeed) * Time.deltaTime;
            float clamped = Mathf.Clamp(_cursor.localScale.x, 1f, 2f);
            _cursor.localScale = new Vector3(clamped, clamped, clamped);
        }
    }

    public void SetCursorImage(Sprite s)
    {
        _cursor.GetComponentInChildren<SpriteRenderer>().sprite = s;
    }

    public void ResetCursorRotScal()
    {
        _cursor.localRotation = Quaternion.identity;
        _cursor.localScale = new Vector3(1, 1, 1);
    }

    public Vector3 GetSpeed()
    {
        return speed;
    }

    #endregion
}
