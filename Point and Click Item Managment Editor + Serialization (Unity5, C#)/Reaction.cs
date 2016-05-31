using UnityEngine;
using System.Collections;
using System.Xml.Serialization;

public enum ReactionType
{
    ChooseType = 0,
    OpenDoor = 1,
    Switch = 2,
    Say = 3,
	SwitchSprite = 4

}

[XmlInclude(typeof(ReactionSay))]
[XmlInclude(typeof(ReactionOpenDoor))]
[XmlInclude(typeof(ReactionSwitchSprite))]

[System.Serializable]
public class Reaction 
{
    public string m_strGOThisID = "";
	public ReactionType m_reactiontypeThis;
	public string m_stringName;

    public Reaction()
    {

    }
    public Reaction(string _strGOID, string _Name, int _TypeID)
    {
        m_strGOThisID = _strGOID;
        m_reactiontypeThis = (ReactionType)_TypeID;
        m_stringName = _Name;
    }

    public Reaction(string _strGOID, string _Name, ReactionType _Type)
	{
        m_strGOThisID = _strGOID;
		m_reactiontypeThis = _Type;
		m_stringName = _Name;
	}

	public virtual void React()
    {

    }

	public string Name
	{
		get
		{
			return m_stringName;
		}
	}

	public ReactionType TypeID
	{
		get
		{
			return m_reactiontypeThis;
		}
	}

    public GameObject GetGO()
    {
            return UniqueIDRegistry.GetInstanceGO(m_strGOThisID);
    }
}
