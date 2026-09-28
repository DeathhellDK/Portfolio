using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using TMPro;

public class GameSceneManager : MonoBehaviour
{
    public static GameSceneManager instance = null;

    public GameObject BenConversationPanel;
    public GameObject TomConversationPanel;
    public GameObject JackConversationPanel;

    public GameObject ChoicePanel;
    public GameObject NoNPCAnxietyPanel;
    public GameObject NoEnergyPanel;

    public GameObject RightChoiceAdvicePanel;
    public GameObject WrongChoiceAdvicePanel;

    public GameObject RewardPanel;

    public TextMeshProUGUI BenConversationTextDisplay;
    public TextMeshProUGUI TomConversationTextDisplay;
    public TextMeshProUGUI JackConversationTextDisplay;

    public TextMeshProUGUI RightChoiceAdviceText;
    public TextMeshProUGUI WrongChoiceAdviceText;

    public TextMeshProUGUI NPCDialogTextDescription;
    public TextMeshProUGUI _OptionOneBtn;
    public TextMeshProUGUI _OptionTwoBtn;

    public TextMeshProUGUI RewardPanelText;

    public bool NPCBenInteraction;
    public bool NPCTomInteraction;
    public bool NPCJackInteraction;


    #region List
    /// <summary>
    /// Conversation with ben for string
    /// </summary>
    List<string> _ConversationWithBen = new List<string>()
    {
        /// Conversation 1
        "Haizz, there is so much work to do",
        "Why the teacher have to give us so much thing to do?",
        "Is the teacher even sane?",
        "Urr... at this rate I won't be able to do finish it! What to do? What to do?",
        "Save me divine being!!!!",

        /// 5
        "",

        /// Conversation 2
        "I'm stressed.",
        "I have to worry about money issues along with academic stress.",
        "What do I do? Should I drop out and find a job instead?",

        /// 9
        "",

        /// Conversation 3
        "It is my turn for the interview soon...",
        "I'm very nervous... I really need this scholarship.",
        "I need it to continue studying without worrying about my school fees.",

        /// 13
        ""
    };

    /// <summary>
    /// Choice dialog to show ask question base on the conversation
    /// </summary>
    List<string> _ChoicDialogOfBen = new List<string>()
    {
        /// Choice dialog 1
        "Ben is experiencing anxiety issues due to the huge pile of assignments that his lecturer has given him in class recently. What do you think he should do to relieve his stress?",

        /// Choice dialog 2
        "You meet Ben again. This time he mentions the financial woes that he and his family are facing due to the pandemic.",

        /// Choice dialog 3
        "Ben seems tense. You walk up to Ben and you find out he is about to attend an interview for a scholarship. What can you do?",

        ""
    };

    /// <summary>
    /// Right choice for Ben
    /// </summary>
    List<string> _RightChoicewithBen = new List<string>()
    {
        /// Right choice 1
        "Convince Ben to take a breather.",

        /// Right choice 2
        "Convince him to seek advice from a counsellor.",

        /// Right choice 3
        "Calm him down by explaining to him different ways to cope with anxiety such as slowing down and taking deep breaths.",

        ""
    };

    /// <summary>
    /// Wrong choice for Ben 
    /// </summary>
    List<string> _WrongChoicewithBen = new List<string>()
    {
        /// Wrong choice 1
        "Convince him to buckle down and focus on his projects.",

        /// Wrong choice 2
        "Support his decision.",

        /// Wrong choice 3
        "Advice him to skip the interview if he is too nervous.",

        ""
    };

    /// <summary>
    /// Conversation with Tom
    /// </summary>
    List<string> _ConversationWithTom = new List<string>()
    {
        /// Conversation 1
        "I'm so tired.",
        "I'm not sure how to do this work at all!",
        "What am I supposed to do when I don't know how to do it?!",
        "I'm so doomed!",
        "What should I do?",

        /// 5
        "",

        /// Conversation 2
        "The latest release version of minecraft just came out.",
        "I really want to play it but I have a huge amount of assignments.",
        "Video games help me release my stress...",
        "What should I do?",

        /// 10
        "",

        /// Conversation 3
        "I am almost done with my assignment!",
        "But there's this really difficult question that I can't solve",
        "My friend showed me his work to help me.",
        "Should I copy his work?",

        /// 15
        ""
    };

    /// <summary>
    /// Choice dialog to show ask question base on the convesation 
    /// </summary>
    List<string> _ChoiceDialogOfTom = new List<string>()
    {
        "Tom is stressed out and facing anxiety issues because he is not sure how to do his work. What advice can you give him?",
        "Tom has to decide between doing his assignments and playing the latest release of Minecraft but playing video games helps him to relieve stress. What advice can you give him?",
        "Tom has to decide between copying his friends' work which might result in plagarism and negatively affect his results or finding an alternative. What advice can you give him?"
    };

    /// <summary>
    /// Right choice for Tom
    /// </summary>
    List<string> _RightChoicewithTom = new List<string>()
    {
        "Let him take a breath first. Find out what problem he has in the topic and try to solve it together with him.",
        "Tell him to moderate his playtime and balance it with his assignments.",
        "Tell him to seek help or consult his tutors or teachers for help."
    };

    /// <summary>
    /// Wrong choice for Tom
    /// </summary>
    List<string> _WrongChoicewithTom = new List<string>()
    {
        "Laugh at him and call him stupid",
        "Support him and tell him to proceed to play the latest release of Minecraft and burn all his assignment.",
        "Give him the link to others people work and report him to the teacher"
    };

    /// <summary>
    /// Conversation with Jack
    /// </summary>
    List<string> _ConversationwithJack = new List<string>()
    {
        "I'm so depressed.",
        "My grandmother passed away recently. ",
        "There's too much going on in my head right now.",
        "I don't know what to do!",
        "What should I do?",

        /// 5
        "",

        "I recently got invited to a special programme for gifted students.",
        "But...My family cannot afford the fees...",
        "What do I do? I don't want to burden my family but also not miss out on this...",

        /// 9
        "",

        "I am now in the gifted programme!",
        "However, the students there look down on me...",
        "What do I do?",

        /// 13
        "",
    };

    /// <summary>
    /// Choice dialog to show ask question base on the convesation 
    /// </summary>
    List<string> _ChoicDialogOfJack = new List<string>()
    {
        "Jack is facing alot of stress from both home and school. What can you do to help him?",
        "Jack has to decide between attending the gifted programme or missing out on it and potentially not being invited again. What can you do to help him?",
        "Jack is being ridiculed because of his background and is feeling down. What can you do to help him?"
    };

    /// <summary>
    /// Right Choice for Jack
    /// </summary>
    List<string> _RightChoicewithJack = new List<string>()
    {
        "Offer him your support and help him get professional help.",
        "Tell him to seek finanical help from the school or guidance from counsellors.",
        "Explain to him how our backgrounds don't define us."
    };

    /// <summary>
    /// Wrong Choice for Jack
    /// </summary>
    List<string> _WrongChoicewithJack = new List<string>()
    {
        "Give him your own advice.",
        "Tell him to pass on it as it is a waste of time.",
        "Tell him to fight the people who ridiculed him."
    };
    #endregion

    /// <summary>
    /// Right Choice advice for Ben
    /// </summary>
    List<string> _RightChoiceAdviceForBen = new List<string>()
    {
        "You have chosen the correct option. Studies show that by taking breaks, it can help reduce stress and maintain consistent performance",
        "You have chosen the correct option. You should always provide positive feedback and seek alternative opinion and not support Ben to make decisions rashly.",
        "You have chosen the correct option. You should try to share tips onhow to stay calm if you see someone being nervous or panicking."
    };

    /// <summary>
    /// Right Choice advice for Tom
    /// </summary>
    List<string> _RightChoiceAdviceForTom = new List<string>()
    {
        "You have chosen the correct option. Letting him take a breath will help him to calm down. Moreover, guiding him through questions will help alleviate the stress that he is under.",
        "You have chosen the correct option. We should always try to balance our school with our daily life so it does not negatively affect our mental health.",
        "You have chosen the correct option. We should never take the easy way and copy other people's hard work. Furthermore, this can cause anxiety in people who do cheat."
    };

    /// <summary>
    /// Right Choice advice for Jack
    /// </summary>
    List<string> _RightChoiceAdviceForJack = new List<string>()
    {
        "You have chosen the correct option. Offering him help is the first step to help him. However, you are not a professional so you should refer him to people who can actually help him.",
        "You have chosen the correct option. We should never make rash decisions which might affect your future. Getting other opinions will give you more perspective and put less pressure on yourself.",
        "You have chosen the correct option. We are not defined by our background and status, by telling Jack to come to terms with his situation, he will feel that someone understands him and is there for him."
    };

    /// <summary>
    /// Wrong Choice advice for Ben
    /// </summary>
    List<string> _WrongChoiceAdviceForBen = new List<string>()
    {
        "You have chosen the wrong option. Over-studying or overworking will lead to people burning out and only increases their stress levels.",
        "You have chosen the wrong option. Running away from your problems will not solve them. This will only cause the anxiety to return if they run into the same problem.",
        "You have chosen the wrong option. Making rash decisions is never a good idea."
    };

    /// <summary>
    /// Wrong Choice for Tom
    /// </summary>
    List<string> _WrongChoiceAdviceForTom = new List<string>()
    {
        "You have chosen the wrong option. Tom does not know what to do, what he needs the most is help.",
        "You have chosen the wrong option. Balance is the key to a stress-free life.",
        "You have chosen the wrong option. Doing something that is wrong will always result in stress." + "It is best to do it the correct way so that your conscience will not cause you more stress."
    };

    /// <summary>
    /// Wrong Choice for Jack
    /// </summary>
    List<string> _WrongChoiceAdviceForJack = new List<string>()
    {
        "You have chosen the wrong option. Jack is facing problems that you cannot solve. Hence, it is better to just offer your support and try to find him the help he needs.",
        "You have chosen the wrong option. Don't be quick and rash to reject opportunities because we can't meet the requirements. Think out of the box, will he regret rejecting this opportunity in the future?",
        "You have chosen the wrong option. It is wrong to start fights and starting a fight could get Jack in trouble and make him lose his position in the programme. Instead think rationally and don't let emotions take over."
    };

    /// <summary>
    /// Reward List Display
    /// </summary>
    List<string> _RewardPlayerEnergy = new List<string>()
    {
        "You have helped Ben successfully! You have been rewarded with 50 energy!",
        "You have helped Tom successfully! You have been rewarded with 50 energy!",
        "You have helped Jack successfully! You have been rewarded with 50 energy!"
    };

    void Start()
    {
        if (instance == null)
        {
            instance = this;
        }
        else if (instance != this)
        {
            Destroy(gameObject);
        }
    }

    public int CheckPanel(int counterNo)
    {
        int TagTracker = counterNo;

        if (TagTracker == 0)
        {
            BenConversationPanel.SetActive(true);
            NPCBenInteractionWithPlayer();
            return TagTracker;
        }
        else if (TagTracker == 1)
        {
            TomConversationPanel.SetActive(true);
            NPCTomInteractionWithPlayer();
            return TagTracker;
        }
        else if (TagTracker == 2)
        {
            JackConversationPanel.SetActive(true);
            NPCJackInteractionWithPlayer();
            return TagTracker;
        }
        else
        {
            BenConversationPanel.SetActive(false);
            TomConversationPanel.SetActive(false);
            JackConversationPanel.SetActive(false);
            return TagTracker;
        }
    }

    #region Ben

    public string NPCBenInteractionWithPlayer()
    {
        if (CharacterManager.BenNPCDialogCounterTracker == 0)
        {
            return BenConversationTextDisplay.text = _ConversationWithBen[0];
        }
        else if (CharacterManager.BenNPCDialogCounterTracker == 1)
        {
            return BenConversationTextDisplay.text = _ConversationWithBen[1];
        }
        else if (CharacterManager.BenNPCDialogCounterTracker == 2)
        {
            return BenConversationTextDisplay.text = _ConversationWithBen[2];
        }
        else if (CharacterManager.BenNPCDialogCounterTracker == 3)
        {
            return BenConversationTextDisplay.text = _ConversationWithBen[3];
        }
        else if (CharacterManager.BenNPCDialogCounterTracker == 4)
        {
            return BenConversationTextDisplay.text = _ConversationWithBen[4];
        }
        else if (CharacterManager.BenNPCDialogCounterTracker == 5)
        {
            PlayerChoiceNPCBenOption();
            BenConversationPanel.SetActive(false);
            return BenConversationTextDisplay.text = "";
        }
        else if (CharacterManager.BenNPCDialogCounterTracker == 6)
        {
            return BenConversationTextDisplay.text = _ConversationWithBen[6];
        }
        else if (CharacterManager.BenNPCDialogCounterTracker == 7)
        {
            return BenConversationTextDisplay.text = _ConversationWithBen[7];
        }
        else if (CharacterManager.BenNPCDialogCounterTracker == 8)
        {
            return BenConversationTextDisplay.text = _ConversationWithBen[8];
        }
        else if (CharacterManager.BenNPCDialogCounterTracker == 9)
        {
            PlayerChoiceNPCBenOption();
            BenConversationPanel.SetActive(false);
            return BenConversationTextDisplay.text = "";
        }
        else if (CharacterManager.BenNPCDialogCounterTracker == 10)
        {
            return BenConversationTextDisplay.text = _ConversationWithBen[10];
        }
        else if (CharacterManager.BenNPCDialogCounterTracker == 11)
        {
            return BenConversationTextDisplay.text = _ConversationWithBen[11];
        }
        else if (CharacterManager.BenNPCDialogCounterTracker == 12)
        {
            return BenConversationTextDisplay.text = _ConversationWithBen[12];
        }
        else if (CharacterManager.BenNPCDialogCounterTracker == 13)
        {
            PlayerChoiceNPCBenOption();
            BenConversationPanel.SetActive(false);
            return BenConversationTextDisplay.text = "";
        }
        else
        {
            BenConversationPanel.SetActive(false);
            NoNPCAnxietyPanel.SetActive(true);
            return BenConversationTextDisplay.text = "";
        }
    }

    public void PlayerChoiceNPCBenOption()
    {
        ChoicePanel.SetActive(true);
        if (CharacterManager.BenNPCDialogCounterTracker == 5)
        {
            NPCDialogTextDescription.text = _ChoicDialogOfBen[0];
            _OptionOneBtn.text = _RightChoicewithBen[0];
            _OptionTwoBtn.text = _WrongChoicewithBen[0];
        }
        else if (CharacterManager.BenNPCDialogCounterTracker == 9)
        {
            NPCDialogTextDescription.text = _ChoicDialogOfBen[1];
            _OptionOneBtn.text = _RightChoicewithBen[1];
            _OptionTwoBtn.text = _WrongChoicewithBen[1];
        }
        else if (CharacterManager.BenNPCDialogCounterTracker == 13)
        {
            NPCDialogTextDescription.text = _ChoicDialogOfBen[2];
            _OptionOneBtn.text = _RightChoicewithBen[2];
            _OptionTwoBtn.text = _WrongChoicewithBen[2];
        }
        else
        {
            ChoicePanel.SetActive(false);
            NPCDialogTextDescription.text = _ChoicDialogOfBen[3];
            _OptionOneBtn.text = _RightChoicewithBen[3];
            _OptionTwoBtn.text = _WrongChoicewithBen[3];
        }
    }

    public void BenCounterIncrement()
    {
        CharacterManager.BenNPCDialogCounterTracker++;
        NPCBenInteractionWithPlayer();
    }

    #endregion

    #region Tom

    public string NPCTomInteractionWithPlayer()
    {
        if (CharacterManager.TomNPCDialogCounterTracker == 0)
        {
            return TomConversationTextDisplay.text = _ConversationWithTom[0];
        }
        else if (CharacterManager.TomNPCDialogCounterTracker == 1)
        {
            return TomConversationTextDisplay.text = _ConversationWithTom[1];
        }
        else if (CharacterManager.TomNPCDialogCounterTracker == 2)
        {
            return TomConversationTextDisplay.text = _ConversationWithTom[2];
        }
        else if (CharacterManager.TomNPCDialogCounterTracker == 3)
        {
            return TomConversationTextDisplay.text = _ConversationWithTom[3];
        }
        else if (CharacterManager.TomNPCDialogCounterTracker == 4)
        {
            return TomConversationTextDisplay.text = _ConversationWithTom[4];
        }
        else if (CharacterManager.TomNPCDialogCounterTracker == 5)
        {
            PlayerChoiceNPCTomOption();
            TomConversationPanel.SetActive(false);
            return TomConversationTextDisplay.text = "";
        }
        else if (CharacterManager.TomNPCDialogCounterTracker == 6)
        {
            return TomConversationTextDisplay.text = _ConversationWithTom[6];
        }
        else if (CharacterManager.TomNPCDialogCounterTracker == 7)
        {
            return TomConversationTextDisplay.text = _ConversationWithTom[7];
        }
        else if (CharacterManager.TomNPCDialogCounterTracker == 8)
        {
            return TomConversationTextDisplay.text = _ConversationWithTom[8];
        }
        else if (CharacterManager.TomNPCDialogCounterTracker == 9)
        {
            return TomConversationTextDisplay.text = _ConversationWithTom[9];
        }
        else if (CharacterManager.TomNPCDialogCounterTracker == 10)
        {
            PlayerChoiceNPCTomOption();
            TomConversationPanel.SetActive(false);
            return TomConversationTextDisplay.text = "";
        }

        else if (CharacterManager.TomNPCDialogCounterTracker == 11)
        {
            return TomConversationTextDisplay.text = _ConversationWithTom[11];
        }
        else if (CharacterManager.TomNPCDialogCounterTracker == 12)
        {
            return TomConversationTextDisplay.text = _ConversationWithTom[12];
        }
        else if (CharacterManager.TomNPCDialogCounterTracker == 13)
        {
            return TomConversationTextDisplay.text = _ConversationWithTom[13];
        }
        else if (CharacterManager.TomNPCDialogCounterTracker == 14)
        {
            return TomConversationTextDisplay.text = _ConversationWithTom[14];
        }
        else if (CharacterManager.TomNPCDialogCounterTracker == 15)
        {
            PlayerChoiceNPCTomOption();
            TomConversationPanel.SetActive(false);
            return TomConversationTextDisplay.text = "";
        }
        else
        {
            TomConversationPanel.SetActive(false);
            NoNPCAnxietyPanel.SetActive(true);
            return TomConversationTextDisplay.text = "";
        }
    }

    public void PlayerChoiceNPCTomOption()
    {
        ChoicePanel.SetActive(true);
        if (CharacterManager.TomNPCDialogCounterTracker == 5)
        {
            NPCDialogTextDescription.text = _ChoiceDialogOfTom[0];
            _OptionOneBtn.text = _RightChoicewithTom[0];
            _OptionTwoBtn.text = _WrongChoicewithTom[0];
        }
        else if (CharacterManager.TomNPCDialogCounterTracker == 10)
        {
            NPCDialogTextDescription.text = _ChoiceDialogOfTom[1];
            _OptionOneBtn.text = _RightChoicewithTom[1];
            _OptionTwoBtn.text = _WrongChoicewithTom[1];
        }
        else if (CharacterManager.TomNPCDialogCounterTracker == 15)
        {
            NPCDialogTextDescription.text = _ChoiceDialogOfTom[2];
            _OptionOneBtn.text = _RightChoicewithTom[2];
            _OptionTwoBtn.text = _WrongChoicewithTom[2];
        }
        else
        {
            ChoicePanel.SetActive(false);
            NPCDialogTextDescription.text = _ChoiceDialogOfTom[3];
            _OptionOneBtn.text = _RightChoicewithTom[3];
            _OptionTwoBtn.text = _WrongChoicewithTom[3];
        }
    }

    public void TomCounterIncrement()
    {
        CharacterManager.TomNPCDialogCounterTracker++;
        NPCTomInteractionWithPlayer();
    }

    #endregion

    #region Jack

    public string NPCJackInteractionWithPlayer()
    {
        if (CharacterManager.JackNPCDialogCounterTracker == 0)
        {
            return JackConversationTextDisplay.text = _ConversationwithJack[0];
        }
        else if (CharacterManager.JackNPCDialogCounterTracker == 1)
        {
            return JackConversationTextDisplay.text = _ConversationwithJack[1];
        }
        else if (CharacterManager.JackNPCDialogCounterTracker == 2)
        {
            return JackConversationTextDisplay.text = _ConversationwithJack[2];
        }
        else if (CharacterManager.JackNPCDialogCounterTracker == 3)
        {
            return JackConversationTextDisplay.text = _ConversationwithJack[3];
        }
        else if (CharacterManager.JackNPCDialogCounterTracker == 4)
        {
            return JackConversationTextDisplay.text = _ConversationwithJack[4];
        }
        else if (CharacterManager.JackNPCDialogCounterTracker == 5)
        {
            PlayerChoiceNPCJackOption();
            JackConversationPanel.SetActive(false);
            return JackConversationTextDisplay.text = "";
        }
        else if (CharacterManager.JackNPCDialogCounterTracker == 6)
        {
            return JackConversationTextDisplay.text = _ConversationwithJack[6];
        }
        else if (CharacterManager.JackNPCDialogCounterTracker == 7)
        {
            return JackConversationTextDisplay.text = _ConversationwithJack[7];
        }
        else if (CharacterManager.JackNPCDialogCounterTracker == 8)
        {
            return JackConversationTextDisplay.text = _ConversationwithJack[8];
        }
        else if (CharacterManager.JackNPCDialogCounterTracker == 9)
        {
            PlayerChoiceNPCJackOption();
            JackConversationPanel.SetActive(false);
            return JackConversationTextDisplay.text = "";
        }
        else if (CharacterManager.JackNPCDialogCounterTracker == 10)
        {
            return JackConversationTextDisplay.text = _ConversationwithJack[10];
        }
        else if (CharacterManager.JackNPCDialogCounterTracker == 11)
        {
            return JackConversationTextDisplay.text = _ConversationwithJack[11];
        }
        else if (CharacterManager.JackNPCDialogCounterTracker == 12)
        {
            return JackConversationTextDisplay.text = _ConversationwithJack[12];
        }
        else if (CharacterManager.JackNPCDialogCounterTracker == 13)
        {
            PlayerChoiceNPCJackOption();
            JackConversationPanel.SetActive(false);
            return JackConversationTextDisplay.text = "";
        }
        else
        {
            JackConversationPanel.SetActive(false);
            NoNPCAnxietyPanel.SetActive(true);
            return JackConversationTextDisplay.text = "";
        }
    }

    public void PlayerChoiceNPCJackOption()
    {
        ChoicePanel.SetActive(true);
        if (CharacterManager.JackNPCDialogCounterTracker == 5)
        {
            NPCDialogTextDescription.text = _ChoicDialogOfJack[0];
            _OptionOneBtn.text = _RightChoicewithJack[0];
            _OptionTwoBtn.text = _WrongChoicewithJack[0];
        }
        else if (CharacterManager.JackNPCDialogCounterTracker == 9)
        {
            NPCDialogTextDescription.text = _ChoicDialogOfJack[1];
            _OptionOneBtn.text = _RightChoicewithJack[1];
            _OptionTwoBtn.text = _WrongChoicewithJack[1];
        }
        else if (CharacterManager.JackNPCDialogCounterTracker == 13)
        {
            NPCDialogTextDescription.text = _ChoicDialogOfJack[2];
            _OptionOneBtn.text = _RightChoicewithJack[2];
            _OptionTwoBtn.text = _WrongChoicewithJack[2];
        }
        else
        {
            ChoicePanel.SetActive(false);
            NPCDialogTextDescription.text = _ChoicDialogOfJack[3];
            _OptionOneBtn.text = _RightChoicewithJack[3];
            _OptionTwoBtn.text = _WrongChoicewithJack[3];
        }
    }

    public void JackCounterIncrement()
    {
        CharacterManager.JackNPCDialogCounterTracker++;
        NPCJackInteractionWithPlayer();
    }

    #endregion

    public void OptionOneBtnCheck()
    {
        if (NPCBenInteraction == true)
        {
            MainMenuScript.HealthValue = HealthBarManager.setHealthBarLife(MainMenuScript.HealthValue - 0.49999999f);
            MainMenuScript.BenNPCAnxityLevel -= 30;
            NPCBenInteraction = false;
            ChoicePanel.SetActive(false);
            RightChoiceAdviceBen();
            RewardCheck();
        }
        else if (NPCTomInteraction == true)
        {
            MainMenuScript.HealthValue = HealthBarManager.setHealthBarLife(MainMenuScript.HealthValue - 0.49999999f);
            MainMenuScript.TomNPCAnxityLevel -= 30;
            NPCTomInteraction = false;
            ChoicePanel.SetActive(false);
            RightChoiceAdviceTom();
            RewardCheck();
        }
        else if (NPCJackInteraction == true)
        {
            MainMenuScript.HealthValue = HealthBarManager.setHealthBarLife(MainMenuScript.HealthValue - 0.49999999f);
            MainMenuScript.JackNPCAnxityLevel -= 30;
            NPCJackInteraction = false;
            ChoicePanel.SetActive(false);
            RightChoiceAdviceJack();
            RewardCheck();
        }
    }

    public void OptionTwoBtnCheck()
    {
        if (NPCBenInteraction == true)
        {
            MainMenuScript.HealthValue = HealthBarManager.setHealthBarLife(MainMenuScript.HealthValue - 0.49999999f);
            MainMenuScript.BenNPCAnxityLevel += 30;
            NPCBenInteraction = false;
            ChoicePanel.SetActive(false);
            WrongChoicAdviceBen();
        }
        else if (NPCTomInteraction == true)
        {
            MainMenuScript.HealthValue = HealthBarManager.setHealthBarLife(MainMenuScript.HealthValue - 0.49999999f);
            MainMenuScript.TomNPCAnxityLevel += 30;
            NPCTomInteraction = false;
            ChoicePanel.SetActive(false);
            WrongChoiceAdviceTom();
        }
        else if (NPCJackInteraction == true)
        {
            MainMenuScript.HealthValue = HealthBarManager.setHealthBarLife(MainMenuScript.HealthValue - 0.49999999f);
            MainMenuScript.TomNPCAnxityLevel += 30;
            NPCJackInteraction = false;
            ChoicePanel.SetActive(false);
            WrongChoiceAdviceJack();
        }
    }

    #region Choice panel for Ben

    public void RightChoiceAdviceBen()
    {
        RightChoiceAdvicePanel.SetActive(true);
        if (CharacterManager.BenNPCDialogCounterTracker == 5)
        {
            RightChoiceAdviceText.text = _RightChoiceAdviceForBen[0];
        }
        else if (CharacterManager.BenNPCDialogCounterTracker == 9)
        {
            RightChoiceAdviceText.text = _RightChoiceAdviceForBen[1];
        }
        else if (CharacterManager.BenNPCDialogCounterTracker == 13)
        {
            RightChoiceAdviceText.text = _RightChoiceAdviceForBen[2];
        }
    }

    public void WrongChoicAdviceBen()
    {
        WrongChoiceAdvicePanel.SetActive(true);
        if (CharacterManager.BenNPCDialogCounterTracker == 5)
        {
            WrongChoiceAdviceText.text = _WrongChoiceAdviceForBen[0];
        }
        else if (CharacterManager.BenNPCDialogCounterTracker == 9)
        {
            WrongChoiceAdviceText.text = _WrongChoiceAdviceForBen[1];
        }
        else if (CharacterManager.BenNPCDialogCounterTracker == 13)
        {
            WrongChoiceAdviceText.text = _WrongChoiceAdviceForBen[2];
        }
    }

    #endregion

    #region Choice panel for Tom

    public void RightChoiceAdviceTom()
    {
        RightChoiceAdvicePanel.SetActive(true);
        if (CharacterManager.TomNPCDialogCounterTracker == 5)
        {
            RightChoiceAdviceText.text = _RightChoiceAdviceForTom[0];
        }
        else if (CharacterManager.TomNPCDialogCounterTracker == 10)
        {
            RightChoiceAdviceText.text = _RightChoiceAdviceForTom[1];
        }
        else if (CharacterManager.TomNPCDialogCounterTracker == 15)
        {
            RightChoiceAdviceText.text = _RightChoiceAdviceForTom[2];
        }
    }

    public void WrongChoiceAdviceTom()
    {
        WrongChoiceAdvicePanel.SetActive(true);
        if (CharacterManager.TomNPCDialogCounterTracker == 5)
        {
            WrongChoiceAdviceText.text = _WrongChoiceAdviceForTom[0];
        }
        else if (CharacterManager.TomNPCDialogCounterTracker == 10)
        {
            WrongChoiceAdviceText.text = _WrongChoiceAdviceForTom[1];
        }
        else if (CharacterManager.TomNPCDialogCounterTracker == 15)
        {
            WrongChoiceAdviceText.text = _WrongChoiceAdviceForTom[2];
        }
    }

    #endregion

    #region Choice panel for Jack

    public void RightChoiceAdviceJack()
    {
        RightChoiceAdvicePanel.SetActive(true);
        if (CharacterManager.JackNPCDialogCounterTracker == 5)
        {

            RightChoiceAdviceText.text = _RightChoiceAdviceForJack[0];
        }
        else if (CharacterManager.JackNPCDialogCounterTracker == 9)
        {
            RightChoiceAdviceText.text = _RightChoiceAdviceForJack[1];
        }
        else if (CharacterManager.JackNPCDialogCounterTracker == 13)
        {
            RightChoiceAdviceText.text = _RightChoiceAdviceForJack[2];
        }
    }

    public void WrongChoiceAdviceJack()
    {
        RightChoiceAdvicePanel.SetActive(true);
        if (CharacterManager.JackNPCDialogCounterTracker == 5)
        {
            WrongChoiceAdviceText.text = _WrongChoiceAdviceForJack[0];
        }
        else if (CharacterManager.JackNPCDialogCounterTracker == 9)
        {
            WrongChoiceAdviceText.text = _WrongChoiceAdviceForJack[1];
        }
        else if (CharacterManager.JackNPCDialogCounterTracker == 13)
        {
            WrongChoiceAdviceText.text = _WrongChoiceAdviceForJack[2];
        }
    }

    #endregion

    public void ClosePanel()
    {
        NoNPCAnxietyPanel.SetActive(false);
    }

    public void NoEnoughEnergyClosePanel()
    {
        NoEnergyPanel.SetActive(false);
    }

    public void CloseRightChoicePanel()
    {
        RightChoiceAdvicePanel.SetActive(false);
    }

    public void CloseWrongChoicePanel()
    {
        WrongChoiceAdvicePanel.SetActive(false);
    }

    public void CloseRewardPanel()
    {
        RewardPanel.SetActive(false);
    }

    public void RewardCheck()
    {
        if (MainMenuScript.BenNPCAnxityLevel <= 60)
        {
            RewardPanel.SetActive(true);
            RewardPanelText.text = _RewardPlayerEnergy[0];
            MainMenuScript.HealthValue = HealthBarManager.setHealthBarLife(MainMenuScript.HealthValue + 0.39999999f);
        }
        else if (MainMenuScript.BenNPCAnxityLevel <= 30)
        {
            RewardPanel.SetActive(true);
            RewardPanelText.text = _RewardPlayerEnergy[0];
            MainMenuScript.HealthValue = HealthBarManager.setHealthBarLife(MainMenuScript.HealthValue + 0.39999999f);
        }
        else if (MainMenuScript.BenNPCAnxityLevel <= 0)
        {
            RewardPanel.SetActive(true);
            RewardPanelText.text = _RewardPlayerEnergy[0];
            MainMenuScript.HealthValue = HealthBarManager.setHealthBarLife(MainMenuScript.HealthValue + 0.39999999f);
        }
        else if (MainMenuScript.TomNPCAnxityLevel <= 60)
        {
            RewardPanel.SetActive(true);
            RewardPanelText.text = _RewardPlayerEnergy[1];
            MainMenuScript.HealthValue = HealthBarManager.setHealthBarLife(MainMenuScript.HealthValue + 0.39999999f);
        }
        else if (MainMenuScript.TomNPCAnxityLevel <= 30)
        {
            RewardPanel.SetActive(true);
            RewardPanelText.text = _RewardPlayerEnergy[1];
            MainMenuScript.HealthValue = HealthBarManager.setHealthBarLife(MainMenuScript.HealthValue + 0.39999999f);
        }
        else if (MainMenuScript.TomNPCAnxityLevel <= 0)
        {
            RewardPanel.SetActive(true);
            RewardPanelText.text = _RewardPlayerEnergy[1];
            MainMenuScript.HealthValue = HealthBarManager.setHealthBarLife(MainMenuScript.HealthValue + 0.39999999f);
        }
        else if (MainMenuScript.JackNPCAnxityLevel <= 60)
        {
            RewardPanel.SetActive(true);
            RewardPanelText.text = _RewardPlayerEnergy[2];
            MainMenuScript.HealthValue = HealthBarManager.setHealthBarLife(MainMenuScript.HealthValue + 0.39999999f);
        }
        else if (MainMenuScript.JackNPCAnxityLevel <= 30)
        {
            RewardPanel.SetActive(true);
            RewardPanelText.text = _RewardPlayerEnergy[2];
            MainMenuScript.HealthValue = HealthBarManager.setHealthBarLife(MainMenuScript.HealthValue + 0.39999999f);
        }
        else if (MainMenuScript.JackNPCAnxityLevel <= 0)
        {
            RewardPanel.SetActive(true);
            RewardPanelText.text = _RewardPlayerEnergy[2];
            MainMenuScript.HealthValue = HealthBarManager.setHealthBarLife(MainMenuScript.HealthValue + 0.39999999f);
        }
    }




}