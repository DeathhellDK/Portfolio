using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.SceneManagement;
using TMPro;

public class CharacterManager : MonoBehaviour
{
    public Rigidbody rb;
    public Joystick joystick;

    public GameObject NPCAnxityDisplayList;

    public TextMeshProUGUI BenAnxityDisplayTxt;
    public TextMeshProUGUI TomAnxityDisplayTxt;
    public TextMeshProUGUI JackAnxityDisplayTxt;

    public static int BenNPCDialogCounterTracker = 0;
    public static int TomNPCDialogCounterTracker = 0;
    public static int JackNPCDialogCounterTracker = 0;

    public string EndGameScene;
    public string SchoolGameScene;
    public string HomeGameScene;

    public int Counterno = 0;
    public int[] AnxityStoreageCheck = { MainMenuScript.BenNPCAnxityLevel, MainMenuScript.TomNPCAnxityLevel, MainMenuScript.JackNPCAnxityLevel };

    public float speed = 5.0f;

    public bool _ConditionCollision;
    

    // Start is called before the first frame update
    void Start()
    {
        ///Get the rigidboday compoment of the player gameObject
        rb = GetComponent<Rigidbody>();

        _ConditionCollision = false;
    }

    void Update()
    {
        if (Input.GetKeyDown("space"))
        {
            TimeSystem.day++;
        }

        HealthBarManager.setHealthBarLife(MainMenuScript.HealthValue);

        if(MainMenuScript.NPCClear == 3)
        {
            SceneManager.LoadScene(EndGameScene);
        }

        AnxityLevelDisplay();

        Debug.Log(MainMenuScript.NPCClear);
    }

    // Update is called once per frame
    void FixedUpdate()
    {
        MovementCharacter();
    }

    void MovementCharacter()
    {
        /// Player will be able to move up and down.
        float HorizontalMovement = joystick.Horizontal;

        ///Player will be able to move left and right.
        float VerticalMovement = joystick.Vertical;

        Quaternion quaternion1 = Quaternion.Euler(0, 90, 0);
        Quaternion quaternion2 = Quaternion.Euler(0, -90, 0);
        Quaternion quaternion3 = Quaternion.Euler(0, 0, 0);
        Quaternion quaternion4 = Quaternion.Euler(0, -180, 0);

        if (HorizontalMovement > 0)
        {
            transform.localRotation = quaternion1;
        }
        else if (HorizontalMovement < 0)
        {
            transform.localRotation = quaternion2;
        }
        if (VerticalMovement > .5f)
        {
            transform.localRotation = quaternion3;
        }
        else if (VerticalMovement < -.5f)
        {
            transform.localRotation = quaternion4;
        }

        ///Player will be moving up,down,left,right at the speed of 5m/s
        rb.velocity = new Vector3(HorizontalMovement * speed, 0, VerticalMovement * speed);
    }

    private void OnCollisionExit(Collision collision)
    {
        if(collision.gameObject.tag == "AnxietyNPCBen")
        {
            if (BenNPCDialogCounterTracker == 13)
            {
                MainMenuScript.NPCClear += 1;

                BenNPCDialogCounterTracker++;
            }
            else
            {
                BenNPCDialogCounterTracker++;
            }
        }
        
        if(collision.gameObject.tag == "AnxietyNPCTom")
        {
            if (TomNPCDialogCounterTracker == 15)
            {
                MainMenuScript.NPCClear += 1;

                TomNPCDialogCounterTracker++;
            }
            else
            {
                TomNPCDialogCounterTracker++;
            }

        }


        if (collision.gameObject.tag == "AnxietyNPCJack")
        {
            if (JackNPCDialogCounterTracker == 13)
            {
                MainMenuScript.NPCClear += 1;

                JackNPCDialogCounterTracker++;
            }
            else
            {
                JackNPCDialogCounterTracker++;
            }
        }
        
        
    }

    private void OnCollisionEnter(Collision collision)
    {
        if (collision.gameObject.tag == "AnxietyNPCBen")
        {
            Counterno = 0;

            if (HealthBarManager.getHealthBarLife() > 0.2f)
            {
                GameSceneManager.instance.NPCBenInteraction = true;
                GameSceneManager.instance.CheckPanel(Counterno);
            }
            else
            {
                GameSceneManager.instance.NoEnergyPanel.SetActive(true);
            }
        }

        else if (collision.gameObject.tag == "AnxietyNPCTom")
        {
            Counterno = 1;
            if (HealthBarManager.getHealthBarLife() > 0.2f)
            {
                GameSceneManager.instance.NPCTomInteraction = true;
                GameSceneManager.instance.CheckPanel(Counterno);
            }
            else
            {
                GameSceneManager.instance.NoEnergyPanel.SetActive(true);
            }
        }

        else if (collision.gameObject.tag == "AnxietyNPCJack")
        {
            Counterno = 2;
            if (HealthBarManager.getHealthBarLife() > 0.2f)
            {
                GameSceneManager.instance.NPCJackInteraction = true;
                GameSceneManager.instance.CheckPanel(Counterno);
            }
            else
            {
                GameSceneManager.instance.NoEnergyPanel.SetActive(true);
            }
        }

        if (collision.gameObject.tag == "Bed")
        {
            MainMenuScript.HealthValue = 1;
            HealthBarManager.setHealthBarLife(MainMenuScript.HealthValue);
            TimeSystem.hour = 7;
            TimeSystem.minute = -1;
            TimeSystem.day++;
        }
        else if (collision.gameObject.tag == "DoorToSchool")
        {
            HealthBarManager.setHealthBarLife(MainMenuScript.HealthValue);
            SceneManager.LoadScene(SchoolGameScene);
        }
        else if (collision.gameObject.tag == "DoorToHome")
        {
            SceneManager.LoadScene(HomeGameScene);
        }
    }

    public void DisplayNpcListofAnxityPeoplePanel()
    {
        NPCAnxityDisplayList.SetActive(true);
    }
   
    public void CloseNpcListofAnxityPeoplePanel()
    {
        NPCAnxityDisplayList.SetActive(false);
    }

    public void AnxityLevelDisplay()
    {
        BenAnxityDisplayTxt.text = "" + MainMenuScript.BenNPCAnxityLevel;
        TomAnxityDisplayTxt.text = "" + MainMenuScript.TomNPCAnxityLevel;
        JackAnxityDisplayTxt.text = "" + MainMenuScript.JackNPCAnxityLevel;
    }

}

