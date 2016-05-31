using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public enum InteractionType
{
    LookAt = 0,
    Use,
    Talk 
}

[System.Serializable]
public class Interaction
{
    //InteractionType of this Interaction; Default is LookAt
    public InteractionType m_interactiontypeThis = InteractionType.LookAt;

    //Item needed for this interaction; null means no item is needed for this interaction
	public string m_strToolID = "";
    
    //Text to be shown for the playercharacter; Reactionary Text is shown in reactions
	public string m_stringInteractionText = "";

    //List of reactions that are caused by this
    public List<Reaction> m_listireactionEffects;

	public string m_strGOThisID = "";

	// Use this for initialization
	public Interaction()
    {
		if(m_listireactionEffects == null)
        	m_listireactionEffects = new List<Reaction>();
	}

	public Interaction(string _GOID, InteractionType _type, Item _tool, string _text, Reaction[] _reactions)
	{
		m_strGOThisID = _GOID;
		m_interactiontypeThis = _type;
        if(_tool != null)
		    m_strToolID = _tool.gameObject.GetComponent<UniqueID>().m_strUniqueId;
		m_stringInteractionText = _text;
		if (m_listireactionEffects == null)
		{
			m_listireactionEffects = new List<Reaction> ();
            if(_reactions != null)
            {
                for (int i = 0; i < _reactions.Length; ++i)
                {
                    m_listireactionEffects.Add(_reactions[i]);
                }
            }
		}
	}
	
    public virtual void Interact()
    {
        //Show InteractionText
        foreach(Reaction _reaction in m_listireactionEffects)
        {
            _reaction.React();
        }
    }

	public void AddReaction(Reaction _reaction)
	{
		m_listireactionEffects.Add (_reaction);
	}

    public void ReplaceReactionAt(int _index, Reaction _new)
    {
        m_listireactionEffects.Insert(_index, _new);
        m_listireactionEffects.RemoveAt(_index+1);

    }

	public void RemoveReaction(Reaction _reaction)
	{
		m_listireactionEffects.Remove (_reaction);
	}

    public void RemoveReactionAt(int _index)
    {
        m_listireactionEffects.RemoveAt(_index);
    }

	public void RemoveLastReaction()
	{
		m_listireactionEffects.RemoveAt(m_listireactionEffects.Count-1);
	}

	public Reaction GetReactionAt(int _iIndex)
	{
		return m_listireactionEffects [_iIndex];
	}

	public int GetReactionCount()
	{
		return m_listireactionEffects.Count;
	}

    public InteractionType Type
    {
        get
        {
            return m_interactiontypeThis;
        }

        set
        {
            m_interactiontypeThis = value;
        }
    }

	public string Text
	{
		get
		{
			return m_stringInteractionText;
		}

		set
		{
			m_stringInteractionText = value;
		}
	}
    
	public Item GetTool()
	{
        GameObject go = UniqueIDRegistry.GetInstanceGO(m_strToolID);
        if (go != null)
            return go.GetComponent<Item>();
        else
            return null;
    }
    public void SetTool(Item _item)
	{
            m_strToolID = _item.gameObject.GetComponent<UniqueID>().m_strUniqueId;
	}

    public GameObject GetGO()
    {
        return UniqueIDRegistry.GetInstanceGO(m_strGOThisID);
    }
}
