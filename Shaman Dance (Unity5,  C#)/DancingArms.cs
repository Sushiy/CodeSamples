using UnityEngine;
using System.Collections;

public class DancingArms : MonoBehaviour
{
    [SerializeField]
    Transform leftHandT;
    [SerializeField]
    Transform rightHandT;

    [SerializeField]
    Transform leftIdleT;
    [SerializeField]
    Transform rightIdleT;

    [SerializeField]
    Transform torsoT;

    float radius = 3;
    float speed = 10;

    DanceCombos combo;

    Vector2 lVector;
    Vector2 rVector;

	CharacterMovement character;

    // Use this for initialization
    void Start ()
    {
		character = GameObject.FindWithTag ("Player").GetComponent<CharacterMovement> ();
		if (character == null)
			Debug.LogError ("Character with \"Player\" tag not found");
        combo = GetComponent<DanceCombos>();
	}
	
	// Update is called once per frame
	void Update ()
    {
        float lx = Input.GetAxis("Horizontal");
        float ly = Input.GetAxis("Vertical");

        float rx = Input.GetAxis("Rx");
        float ry = -Input.GetAxis("Ry");
        
		if (character.isTurnedLeft) {
			leftHandT.localPosition = Vector3.Lerp(leftHandT.localPosition, leftIdleT.localPosition + radius * new Vector3(-rx, ry, 0), Time.deltaTime * speed);
			rightHandT.localPosition = Vector3.Lerp(rightHandT.localPosition, rightIdleT.localPosition + radius * new Vector3(-lx, ly, 0), Time.deltaTime * speed);

			lVector = new Vector2(-rx, ry);
			rVector = new Vector2(-lx, ly);
		} else {
			leftHandT.localPosition = Vector3.Lerp(leftHandT.localPosition, leftIdleT.localPosition + radius * new Vector3(lx, ly, 0), Time.deltaTime * speed);
			rightHandT.localPosition = Vector3.Lerp(rightHandT.localPosition, rightIdleT.localPosition + radius * new Vector3(rx, ry, 0), Time.deltaTime * speed);

			lVector = new Vector2(lx, ly);
			rVector = new Vector2(rx, ry);
		}


        
    }

    public bool GetHands(out float left, out float right, out Vector2 l, out Vector2 r)
    {
        left = 0;
        right = 0;
        l = new Vector2();
        r = new Vector2();
        if (lVector.magnitude > 0.8f && rVector.magnitude > 0.8f)
        {
            left = Vector3.Angle(transform.up, lVector);
            left = (lVector.y > 0) || (lVector.x > 0) ? left : -left;
            left = (left + 360) % 360;
            right = Vector3.Angle(transform.up, rVector);
            right = (rVector.y > 0) || (rVector.x > 0) ? right : -right;
            right = (right + 360) % 360;

            l = lVector;
            r = rVector;
            return true;
        }

        return false;
    }
}
