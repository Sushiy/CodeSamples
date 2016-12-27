using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.Xml.Serialization;
using System.IO;
using System;

public class Interactable : MonoBehaviour
{
    public static string s_filePath = "Items/XML/Interactables/";
    public List<Interaction> m_listInteractions;

    private TextAsset itemXMLFile;       //The Textasset to load the XMLfile into

    private void Awake()
    {

    }

	// Use this for initialization
	private void Start ()
    {
        m_listInteractions = new List<Interaction>(); 
        if (itemXMLFile == null)
        {
            itemXMLFile = Resources.Load(s_filePath + this.GetComponent<UniqueID>().m_strUniqueId, typeof(TextAsset)) as TextAsset;
        }

        LoadFromXML();
	}
	
	public void Interact(Item _itemTool)
	{
		foreach (Interaction _i in m_listInteractions) 
		{
			if(_i.Type == InteractionType.Use && _i.Tool == _itemTool)
				_i.Interact ();
		}
	}

	public void AddInteraction(Interaction _interaction)
	{
		m_listInteractions.Add (_interaction);
	}

	public void RemoveInteraction(Interaction _interaction)
	{
		m_listInteractions.Remove(_interaction);
	}

    //Now lets create our function to load all of the items
    private void LoadFromXML()
    {
        //We create our XMLSerializer like normal
        XmlSerializer serializer = MakeNewXMLSerializer();

        if (itemXMLFile != null)
        {
            //Here we use a StringReader. This puts the string from the TextAsset into a format that can be deserialized using the XMLSerializer
            using (StringReader reader = new StringReader(itemXMLFile.text))
            {
                //Then we can just assign the items list from here
                m_listInteractions = serializer.Deserialize(reader) as List<Interaction>;
            }
            ReassignUniqueID();
            FinalizeDeserialization(this);

        }
        else
            Debug.Log("xmlfile not found for:" + this.ToString() + "/" + this.GetInstanceID());
    }

    public static void FinalizeDeserialization(Interactable _interactable)
    {
        ReactionSay say;
        ReactionOpenDoor door;
        ReactionSwitchSprite sprite;
        foreach (Interaction i in _interactable.m_listInteractions)
        {
            for (int r = 0; r < i.GetReactionCount(); r++)
            {
                try
                {
                    say = (ReactionSay)i.GetReactionAt(r);
                    i.ReplaceReactionAt(r, say);
                }
                catch (System.Exception e)
                { }
                try
                {
                    door = (ReactionOpenDoor)i.GetReactionAt(r);
                    i.ReplaceReactionAt(r, door);
                }
                catch (System.Exception e)
                { }
                try
                {
                    sprite = (ReactionSwitchSprite)i.GetReactionAt(r);
                    i.ReplaceReactionAt(r, sprite);
                }
                catch (System.Exception e)
                { }
            }
        }
    }

    public void ReassignUniqueID()
    {
        for (int i = 0; i < m_listInteractions.Count; i++)
        {
            Interaction interaction = m_listInteractions[i];
            interaction.m_strGOThisID = GetComponent<UniqueID>().m_strUniqueId;
            for (int j = 0; j < interaction.GetReactionCount(); j++)
            {
                interaction.GetReactionAt(j).m_strGOThisID = interaction.m_strGOThisID;
            }
        }
    }

    public static XmlSerializer MakeNewXMLSerializer()
    {
        // Each overridden field, property, or type requires 
        // an XmlAttributes instance.
        XmlAttributes attrs = new XmlAttributes();

        // Creates an XmlElementAttribute instance to override the 
        // field that returns Book objects. The overridden field
        // returns Expanded objects instead.
        XmlElementAttribute attr = new XmlElementAttribute();
        
        /*REACTIONSAY*/
        attr.ElementName = "Say";
        attr.Type = typeof(ReactionSay);
        attrs.XmlElements.Add(attr);

        XmlElementAttribute attr1 = new XmlElementAttribute();

        /*REACTIONOPENDOOR*/
        attr1.ElementName = "OpenDoor";
        attr1.Type = typeof(ReactionOpenDoor);
        attrs.XmlElements.Add(attr1);
        XmlElementAttribute attr2 = new XmlElementAttribute();

        /*REACTIONSWITCHSPRITE*/
        attr2.ElementName = "SwitchSpriter";
        attr2.Type = typeof(ReactionSwitchSprite);
        attrs.XmlElements.Add(attr2);

        // Creates the XmlAttributeOverrides instance.
        XmlAttributeOverrides attrOverrides = new XmlAttributeOverrides();

        // Adds the type of the class that contains the overridden 
        // member, as well as the XmlAttributes instance to override it 
        // with, to the XmlAttributeOverrides.
        attrOverrides.Add(typeof(Interaction), "m_listireactionEffects", attrs);
        return new XmlSerializer(typeof(List<Interaction>), attrOverrides);
    }
}
