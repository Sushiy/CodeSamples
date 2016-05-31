using UnityEngine;
using System.Collections;

[RequireComponent(typeof(MeshRenderer))]
[RequireComponent(typeof(MeshFilter))]
public class MeshLine : MonoBehaviour
{
    
    #region Fields / private variables
    private float scale = 100f;

    private float lineWidth = 0.03125f/2;
    private Vector3 startPoint;
    private Vector3 endPoint;

    private Vector3[] vertices;

    private Transform _text;

    private Vector3 tangent;
    
    private float angle;

    private float lineOffsetFactor;
    #endregion

    // Use this for initialization
    void Start ()
    {
        vertices = new Vector3[12];
        _text = transform.GetChild(0);
	}
	
	// Update is called once per frame
	void Update ()
    {
        PositionText();
        CalcMeshVertices();
        GenerateMesh();
	}

    #region Mesh-Calculation methods
    void CalcMeshVertices()
    {
        if (startPoint != null && endPoint != null)
        {
            float thickness = lineWidth;
            Vector3 s, e;
            Vector3 direction = (endPoint - startPoint).normalized;
            tangent = (endPoint - startPoint).normalized;
            tangent *= thickness;
            tangent = new Vector3(-tangent.y, tangent.x, tangent.z);

            if ((endPoint - startPoint).x < 0)
            {
                s = startPoint - lineOffsetFactor * tangent;
                e = endPoint - lineOffsetFactor * tangent;
            }
            else
            {
                s = startPoint + lineOffsetFactor * tangent;
                e = endPoint + lineOffsetFactor * tangent;
            }

            Vector3 s1 = s + tangent * 5;
            vertices[0] = s1;
            Vector3 s2 = s - tangent * 5;
            vertices[1] = s2;
            vertices[2] = s1 + direction * thickness;
            vertices[3] = s2 + direction * thickness;
            vertices[4] = s + tangent + direction * thickness;
            vertices[5] = s - tangent + direction * thickness;


            Vector3 e1 = e + tangent * 5;
            vertices[6] = e1;
            Vector3 e2 = e - tangent * 5;
            vertices[7] = e2;
            vertices[8] = e1 - direction * thickness;
            vertices[9] = e2 - direction * thickness;
            vertices[10] = e + tangent - direction * thickness;
            vertices[11] = e - tangent - direction * thickness;
        }
    }

    void GenerateMesh()
    {
        if (startPoint != null && endPoint != null)
        {
            ClearMesh();

            Mesh m = new Mesh();
            m.name = "Procedural_Line_Mesh";
            m.vertices = vertices;
            int triCount = vertices.Length - 2;
            int[] indices = new int[3*triCount];
            indices[0] = 2;
            indices[1] = 1;
            indices[2] = 0;

            indices[3] = 1;
            indices[4] = 2;
            indices[5] = 3;

            indices[6] = 10;
            indices[7] = 5;
            indices[8] = 4;

            indices[9] = 5;
            indices[10] = 10;
            indices[11] = 11;

            indices[12] = 8;
            indices[13] = 6;
            indices[14] = 9;

            indices[15] = 9;
            indices[16] = 6;
            indices[17] = 7;

            m.triangles = indices;
            m.RecalculateNormals();
            GetComponent<MeshFilter>().mesh = m;
        }
    }
    #endregion

    void PositionText()
    {
        if (startPoint != null && endPoint != null)
        {
            Vector3 lineVector = (endPoint - startPoint);
            _text.GetComponent<TextMesh>().text = (Vector3.Distance(startPoint, endPoint) * scale).ToString("F1") + "m";



            Vector3 rotationTargetVector;
            //if the line is a sidewaysvector
            bool up;
            if (lineVector.x < 0)
            {
                rotationTargetVector = -tangent * 2;
                up = lineVector.y > 0? false : true;
            }
            else
            {
                rotationTargetVector = tangent * 2;
                up = lineVector.y > 0 ? true : false;
            }
            
            angle = Vector3.Angle(Vector3.up, rotationTargetVector);
            Vector3 eulerAngles = new Vector3(0, 0, angle);
            eulerAngles *= up ? 1 : -1;
            _text.localRotation = Quaternion.Euler(eulerAngles);

            _text.position = startPoint + lineVector / 2 + rotationTargetVector * 5;
        }
    }

    #region HelperMethods (Getters, Setters, etc)
    public void SetStart(Vector3 pos)
    {
        if(pos != startPoint)
        {
            startPoint = pos;
            CalcMeshVertices();
            GenerateMesh();
           // Debug.Log("start");
        }
    }

    public void SetEnd(Vector3 pos)
    {
        if(pos != endPoint)
        {
            endPoint = pos;
            CalcMeshVertices();
            GenerateMesh();
        }
    }

    public Vector3 GetStart()
    {
        return startPoint;
    }

    public Vector3 GetEnd()
    {
        return endPoint;
    }

    public void ClearPoints()
    {
        startPoint = Vector3.zero;
        endPoint = Vector3.zero;
    }

    public void ClearMesh()
    {
        GetComponent<MeshFilter>().mesh = null;

    }

    public void OnDrawGizmos()
    {
        Gizmos.color = Color.blue;
        Gizmos.DrawRay(startPoint, tangent*10);
    }

    public void SetLineOffsetFactor(float factor)
    {
        lineOffsetFactor = factor;
    }
    #endregion
}
