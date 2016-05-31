using UnityEngine;
using UnityEditor;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Xml.Serialization;


//This Window manages the editing of Interactables
//Here the user can add Interactions, 

public class EditInteractableWindow : EditorWindowSingleton<EditInteractableWindow>
{
    private static Interactable s_interactableEditing;  //The currently selected Interactable. This is automatically changed through the selection in hierarchy.

    private static List<bool> s_bInteractionFoldouts; //If the Interactiondetails (Type, Tool, Text etc.) is currently folded out.
    private Vector2 scrollPosition;

    #region AddInteraction                
    private bool m_bCreatingInteraction = false;        //If the Menu to add an Interaction is currently open
	private InteractionType m_interactionType;          //The Type of the Interaction we want to add to the Interactable.
    private string m_strInteractionName;                   //The Display name for the Interaction
	private Item m_itemTool;                            //The Item that is needed to trigger this Interaction if it is a USE interaction for all other types this is null
	private string m_strText;                           //The standard Text that the character says if this Interaction is triggered. Should this be a dialogue Reaction?
    #endregion

    #region AddReaction
    private static List<bool> s_listbAddReaction;             
    private ReactionType m_reactionType = ReactionType.ChooseType;                  //The Type of Reaction we want to add to the Interaction
    private string m_stringReactionName;                //The Name of the Reaction we want to add to the Interaction
    private Door m_doorTarget;
    private string m_strSayText;
    #endregion

    #region Serialization
    private static readonly string s_filePath = Application.dataPath + "/Resources/" + Interactable.s_filePath;     //The path I want to save all the interactables to
    private static string s_fileName = "fileNameNotSet.xml";                                        //The filename for the selected Interactable 
    #endregion

    GUIStyle m_styleBox;
    GUIStyle m_styleNoPad;
    GUIStyle m_styleHR;

    //This opens and focusses the window through the Singleton class (Always next to SceneView)
    public static void ShowWindow(Interactable _interactable)
    {
		SetInteractable(_interactable);
        EditInteractableWindow window = EditInteractableWindow.Instance;
        window.Focus();
        window.minSize = new Vector2(600, 337);
        window.titleContent = new GUIContent("Interactable");
    }

    //The main function of this window
    private void OnGUI()
    {
        m_styleBox = new GUIStyle(GUI.skin.box);
        m_styleBox.stretchWidth = true;
        m_styleBox.stretchHeight = true;

        m_styleNoPad = new GUIStyle(GUI.skin.box);
        m_styleNoPad.stretchWidth = true;
        m_styleNoPad.margin = new RectOffset(-1, -2, 0, 0);
        m_styleNoPad.padding = new RectOffset(-1, -2, 0, 0);

        m_styleHR = new GUIStyle(GUI.skin.box);
        m_styleHR.stretchWidth = true;
        m_styleHR.padding = new RectOffset(-1, -2, 0, 0);
        m_styleHR.margin = new RectOffset(-1, -2, 0, 0);
        m_styleHR.fixedHeight = 1;

        float windowWidth = position.width;

        EditorGUILayout.BeginVertical(m_styleNoPad);
        //These two lines show the what the currently selected Interactable is.
        EditorGUILayout.LabelField("Currently Selected Interactable");
        EditorGUILayout.ObjectField(s_interactableEditing, typeof(Interactable), true);
        EditorGUILayout.Space();
        EditorGUILayout.EndVertical();
        EditorGUILayout.Space();

        //If the an interactable is selected, the main window content gets rendered.
        if(s_interactableEditing != null)
        {
            // This Line shows the Number of Interactions in the active Interactable. It's also a header
            EditorGUILayout.LabelField("Interactions" + "(" + s_interactableEditing.m_listInteractions.Count + "):", EditorStyles.boldLabel);
            //This shows a collapsable Inspector for each Interaction
            for (int i = 0; i < s_interactableEditing.m_listInteractions.Count; i++)
            {
                GUILayout.Box("", m_styleHR);
                bool bQueueForRemoval = false;
                Interaction interaction = s_interactableEditing.m_listInteractions[i];
                EditorGUILayout.BeginHorizontal();
                //This is the basic Line which contains the InteractionType
                s_bInteractionFoldouts[i] = EditorGUILayout.Foldout(s_bInteractionFoldouts[i], new GUIContent(interaction.Type.ToString() + ((interaction.Type == InteractionType.Use && interaction.GetTool() != null) ? "_" + interaction.GetTool().ToString():"") + "\t| #Reactions: " + interaction.GetReactionCount()));

                //An edit button that allows the user to change the settings of the selected Interaction
                if (GUILayout.Button("Edit", EditorStyles.miniButtonLeft, GUILayout.MaxWidth(50)))
                {
                    //TODO: Button functionality --->> EditInteraction(Interaction _interaction);
                }
                //This Button deletes the Interaction
                if (GUILayout.Button("Delete", EditorStyles.miniButtonRight, GUILayout.MaxWidth(50)))
                {
                    bQueueForRemoval = true;
                }
                EditorGUILayout.EndHorizontal();

                //If the Interaction is folded out, you can see more details for it (Type, Item, standard Text, reactions)
                if (s_bInteractionFoldouts[i])
                {
                    ShowInteractionDetails(interaction, i);
                }

                if(bQueueForRemoval)
                {
                    Debug.Log("Remove");
                    s_interactableEditing.RemoveInteraction(interaction);
                    s_bInteractionFoldouts.RemoveAt(i);
                    s_listbAddReaction.RemoveAt(i);
                }

            }
            GUILayout.Box("", m_styleHR);
            EditorGUILayout.Space();
            EditorGUILayout.Space();

            //If we are currently adding a reaction, this Interface is shown
            if(m_bCreatingInteraction)
            {
                ShowInteractionCreation();
            }
            //If we are not adding a reaction, there is an big + button to add an interaction.
            else if (GUILayout.Button("+"))
            {
                m_bCreatingInteraction = true;
                ResetInteraction();
            }

            /*
            string[] arstringReactionTypes = { "OpenDoor", "Dialogue", "Trigger", "GiveItem" };
            m_iReactionTypeID = EditorGUILayout.Popup(m_iReactionTypeID, arstringReactionTypes);

            m_stringReactionName = EditorGUILayout.TextField(m_stringReactionName);
            */
            EditorGUILayout.BeginHorizontal();
            if (GUILayout.Button("Save"))
            {
                SaveXMLFile();
            }
            if (GUILayout.Button("Load"))
            {
                LoadXMLFile();
            }
            if(GUILayout.Button("Close"))
            {
                this.Close();
            }
            EditorGUILayout.EndHorizontal();
        }

        else
        {
            EditorGUILayout.HelpBox("The currently selected GameObject does not have an Interactable Component", MessageType.Warning);
        }
    }

    private void ShowInteractionDetails(Interaction _interaction, int _index)
    {
        EditorGUILayout.BeginHorizontal();
                        EditorGUILayout.BeginVertical();
                            EditorGUILayout.BeginHorizontal();
                                EditorGUILayout.PrefixLabel("\t Type:");
                                EditorGUILayout.LabelField(_interaction.Type.ToString());
                            EditorGUILayout.EndHorizontal();

                            if (_interaction.Type == InteractionType.Use)
                            {
                                EditorGUILayout.BeginHorizontal();
                                    EditorGUILayout.PrefixLabel("\t Tool:");
                                    if(_interaction.GetTool() != null)
                                        EditorGUILayout.LabelField(_interaction.GetTool().ToString());
                                EditorGUILayout.EndHorizontal();
                            }

                            EditorGUILayout.BeginHorizontal();
                                EditorGUILayout.PrefixLabel("\t Text:");
                                EditorGUILayout.LabelField(_interaction.Text);
                            EditorGUILayout.EndHorizontal();
                        EditorGUILayout.EndVertical();

                        EditorGUILayout.BeginVertical();

                        EditorGUILayout.BeginHorizontal(EditorStyles.toolbarButton);
                        EditorGUILayout.LabelField("Reactions", EditorStyles.boldLabel);
                        //This button should pop up or foldout an interface to add Reactions with
                        if (GUILayout.Button("Add Reaction", EditorStyles.miniButton, GUILayout.MaxWidth(80)))
                        {
                            s_listbAddReaction[_index] = true;
                        }
                        EditorGUILayout.EndHorizontal();
                        
                        if(s_listbAddReaction[_index])
                        {
                            EditorGUILayout.BeginHorizontal();
                            EditorGUILayout.PrefixLabel("Type");
                            m_reactionType = (ReactionType)EditorGUILayout.EnumPopup(m_reactionType);

                            switch(m_reactionType)
                            {
                                case ReactionType.OpenDoor:
                                    
                                    EditorGUILayout.PrefixLabel("Target");
                                    m_doorTarget = (Door)EditorGUILayout.ObjectField(m_doorTarget, typeof(Door), true);
                                    if (GUILayout.Button("Add Reaction"))
                                    {
                                        _interaction.AddReaction(new ReactionOpenDoor(_interaction.GetGO().GetComponent<UniqueID>().m_strUniqueId, m_doorTarget));
                                    }
                                    break;

                                case ReactionType.Say:
                                    EditorGUILayout.PrefixLabel("Dialogue Text");
                                    m_strSayText = EditorGUILayout.TextField(m_strSayText);
                                    if (GUILayout.Button("Add Reaction"))
                                    {
                                        _interaction.AddReaction(new ReactionSay(_interaction.GetGO().GetComponent<UniqueID>().m_strUniqueId, m_strSayText));
                                    }
                                    break;

                                default:


                                    EditorGUILayout.LabelField("This Type cannot be created! Choose another.");
                                    break;
                            }
                            EditorGUILayout.EndHorizontal();
                        }

                        scrollPosition = EditorGUILayout.BeginScrollView(scrollPosition, false, true, GUIStyle.none, GUI.skin.verticalScrollbar, m_styleBox, GUILayout.MaxHeight(GUI.skin.label.lineHeight * 6));
                        if(_interaction.GetReactionCount() > 0)
                        {
                            for (int reactionIndex = 0; reactionIndex < _interaction.GetReactionCount(); reactionIndex++)
                            {
                                EditorGUILayout.BeginHorizontal(GUILayout.MaxWidth(position.width/3));
                                    Reaction r = _interaction.GetReactionAt(reactionIndex);
                                    EditorGUILayout.LabelField(r.Name, GUILayout.MaxWidth(80));
                                    switch(r.m_reactiontypeThis)
                                    {
                                        case ReactionType.OpenDoor:
                                            EditorGUILayout.PrefixLabel("Target");
                                            try
                                            {
                                                ReactionOpenDoor door = (ReactionOpenDoor)r;
                                                EditorGUILayout.LabelField(UniqueIDRegistry.GetInstanceGO(door.m_strTargetID).ToString());
                                            }
                                            catch (System.Exception e)
                                            {

                                            }
                                            break;

                                        case ReactionType.Say:
                                            EditorGUILayout.PrefixLabel("Dialogue Text");
                                            try
                                            {
                                                ReactionSay say = (ReactionSay)r;
                                                EditorGUILayout.LabelField(say.m_stringSayText);
                                            }
                                            catch (System.Exception e)
                                            {
                                            }
                                            break;

                                        default:
                                            EditorGUILayout.LabelField("This Type cannot be created! Choose another.");
                                            break;
                                    }
                                    
                                    if(GUILayout.Button("-", EditorStyles.miniButton, GUILayout.MaxWidth(30)))
                                    {
                                        _interaction.RemoveReactionAt(reactionIndex);
                                    }   
                                EditorGUILayout.EndHorizontal();
                            }
                        }
                    
                        EditorGUILayout.EndScrollView();
                        EditorGUILayout.EndVertical();                        
                    EditorGUILayout.EndHorizontal();
    }

    private void ShowInteractionCreation()
    {
        //Enum Dropdown to choose the correct InteractionType
        EditorGUILayout.BeginHorizontal();
        EditorGUILayout.PrefixLabel("Type");
        m_interactionType = (InteractionType)EditorGUILayout.EnumPopup(m_interactionType);

        //If the Type is Use, you can choose a Tool. otherwise tool = null
        if (m_interactionType == InteractionType.Use)
        {
            EditorGUILayout.PrefixLabel("Tool");
            m_itemTool = (Item)EditorGUILayout.ObjectField(m_itemTool, typeof(Item), true);
        }

        EditorGUILayout.EndHorizontal();

        //The standard text for the interaction
        EditorStyles.textField.wordWrap = true;
        EditorGUILayout.BeginHorizontal();
        EditorGUILayout.PrefixLabel("Text");
        m_strText = EditorGUILayout.TextArea(m_strText, GUILayout.MinWidth(200), GUILayout.MinHeight(50));
        EditorGUILayout.EndHorizontal();


        EditorGUILayout.BeginHorizontal();

        //This button actually adds the interaction. This is only possible if the text has been edited.
        if (GUILayout.Button("Add"))
        {
            s_interactableEditing.AddInteraction(new Interaction(s_interactableEditing.GetComponent<UniqueID>().m_strUniqueId, m_interactionType, m_itemTool, m_strText, null));
            s_bInteractionFoldouts.Add(false);
            s_listbAddReaction.Add(false);
            this.Repaint();
            ResetInteraction();
            m_bCreatingInteraction = false;
        }
        GUI.enabled = true;
        if (GUILayout.Button("Cancel"))
        {
            ResetInteraction();
            m_bCreatingInteraction = false;
        }
        EditorGUILayout.EndHorizontal();
    }

    private static void SetInteractable(Interactable _interactable)
    {
        if(_interactable == null)
        {
            s_interactableEditing = null;
            s_fileName = "";
        }
        else
        {
            s_interactableEditing = _interactable;
            s_fileName = s_interactableEditing.GetComponent<UniqueID>().m_strUniqueId + ".xml";
            s_bInteractionFoldouts = new List<bool>();
            s_listbAddReaction = new List<bool>();
            if (s_interactableEditing.m_listInteractions.Count > 0)
            {
                for (int i = 0; i < s_interactableEditing.m_listInteractions.Count; i++)
                {
                    s_bInteractionFoldouts.Add(false);
                    s_listbAddReaction.Add(false);
                }
            }           
        }
    }

    private static void LoadXMLFile()
    {
        //Here we will make sure to initalize the items and item foldouts list
        s_interactableEditing.m_listInteractions = new List<Interaction>();

        //if the file exists we load it, otherwise we dont do anything
        if (File.Exists(s_filePath + s_fileName))
        {

            //lets create our XMLSerializer to handle the item data
            XmlSerializer serializer = Interactable.MakeNewXMLSerializer();

            //Next we create a new FileStream with our file path. Notice that we have the using statement. 
            //This allows us to do whatever we want with the Filestream and know that it will close itself automatically
            using (FileStream file = new FileStream(s_filePath + s_fileName, FileMode.Open))
            {
                try
                {
                    //Lets try to load the data into the items list and set up the foldouts list as well
                    s_interactableEditing.m_listInteractions = serializer.Deserialize(file) as List<Interaction>;
                }
                catch (System.Exception e)
                {

                }
            }
            s_interactableEditing.ReassignUniqueID();
            Interactable.FinalizeDeserialization(s_interactableEditing);
        }

    }

    //Next us our function to save the data to the disc
    private static void SaveXMLFile()
    {
        //First lets check to see if the directory to the file exists, and if it doesn't we create it
        if (!Directory.Exists(s_filePath))
        {
            Directory.CreateDirectory(s_filePath);
        }

        //Then we serialize the list of items into the XML file and refresh the AssetDatabase
        using (FileStream file = new FileStream(s_filePath + s_fileName, FileMode.Create))
        {
            XmlSerializer serializer = Interactable.MakeNewXMLSerializer();
            serializer.Serialize(file, s_interactableEditing.m_listInteractions);

            AssetDatabase.Refresh();
        }
    }

    
    private void ResetInteraction()
    {
        m_interactionType = InteractionType.LookAt;
        m_strInteractionName = "";
        m_itemTool = null;
        m_strText = "";
    }

    public void OnSelectionChange()
    {
        if(s_interactableEditing != null)
            SaveXMLFile();
        if (Selection.activeGameObject == null)
            SetInteractable(null);
        else
        {
            GameObject go = Selection.activeGameObject;
            SetInteractable(go.GetComponent<Interactable>());
            EditInteractableWindow.Instance.Repaint();

            LoadXMLFile();
        }

        ResetInteraction();
        m_bCreatingInteraction = false;
    }


}

