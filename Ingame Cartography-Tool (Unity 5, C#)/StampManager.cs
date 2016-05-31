using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class StampManager : MonoBehaviour
{
    [SerializeField]
    private GameObject _stampPrefab;
    private GameObject[] objectPool;
    private int maxObjects = 64;
    private int usedObjects = 0;

    Vector3 removeVector = new Vector3(0, 0, 2.5f);
    [SerializeField]
    private Transform _parentTransform;

	// Use this for initialization
	void Start ()
    {
        objectPool = new GameObject[maxObjects];
        FillPool();
	}
	
	// Update is called once per frame
	void Update ()
    {
	    
	}

    public bool IsFull()
    {
        return usedObjects >= maxObjects;
    }

    public void AddStamp(Vector3 position, Quaternion rotation, float scale, Sprite sprite)
    {
        if (!IsFull())
        {
            objectPool[usedObjects].transform.position = position;
            objectPool[usedObjects].GetComponent<SpriteRenderer>().sprite = sprite;
            objectPool[usedObjects].transform.SetParent(_parentTransform, true);
            objectPool[usedObjects].transform.localScale = new Vector3(scale, scale, scale);
            usedObjects++;
        }
        else
        {
            Debug.Log("Resetting full pool");
            ResetPool();
            AddStamp(position, rotation, scale, sprite);
        }
    }

    void RemoveStampAt(int index)
    {
        objectPool[index].transform.localPosition = removeVector;
    }

    void RemoveAll()
    {
        for(int i = 0; i < objectPool.Length; ++i)
        {
            RemoveStampAt(0);
        }
    }

    void FillPool()
    {
        GameObject g;
        for (int i = 0; i < maxObjects; ++i)
        {
            g = (GameObject)GameObject.Instantiate(_stampPrefab, removeVector, Quaternion.identity);
            g.transform.SetParent(_parentTransform);
            g.transform.transform.localScale = new Vector3(1, 1, 1);
            objectPool[i] = g;
        }
    }

    public void ResetPool()
    {
        RemoveAll();
        usedObjects = 0;
    }
}
