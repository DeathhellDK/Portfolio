using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class HealthBarManager : MonoBehaviour
{
    private static Image _HealthbarImg;
    public Text _HealthCurrentLifePointtxt;
    public int _CurrentLifePoint;

    // Start is called before the first frame update
    void Start()
    {
        /// Get the compoment of the gameobject call Image from the inspector
        _HealthbarImg = GetComponent<Image>();

    }

    // Update is called once per frame
    void Update()
    {
        ///Set the health bar as the return value of the health amount
        setHealthBarLife(getHealthBarLife());

        ///Set the current lifepoint to be the overall multiply by 100
        ///Convert it to integer as it is out of 100%
        _CurrentLifePoint = (int)(_HealthbarImg.fillAmount * 100.0f);

        ///Display the current life point in the textbox
        _HealthCurrentLifePointtxt.text = _CurrentLifePoint.ToString() + "/100";

    }

    /// <summary>
    /// Method to set the lifepoint of the health bar
    /// </summary>
    /// <param name="LifePoint"></param>
    public static float setHealthBarLife(float LifePoint)
    {
        /// Set the current lifepoint to equal to the fill amount value in the imgae
        _HealthbarImg.fillAmount = LifePoint;

        ///Condition if the fill amount is less than 0.2
        if (_HealthbarImg.fillAmount < 0.2f)
        {
            ///Set the color of the health bar to be red
            setHealtBarColorIndicator(Color.red);
            return _HealthbarImg.fillAmount;
        }

        ///Condition if the fill amount is less than 0.5
        else if (_HealthbarImg.fillAmount < 0.5f)
        {
            ///Set the color of the health bar to be yellow
            setHealtBarColorIndicator(Color.yellow);
            return _HealthbarImg.fillAmount;
        }

        ///Condition if it does not match any of the above condition
        else
        {
            ///set the color of the health bar to be green
            setHealtBarColorIndicator(Color.green);
            return _HealthbarImg.fillAmount;
        }
    }

    /// <summary>
    /// Method to get the health value
    /// </summary>
    /// <returns></returns>
    public static float getHealthBarLife()
    {
        ///get the health fill amount as the value
        return _HealthbarImg.fillAmount;
    }

    /// <summary>
    /// Method Set the color of the health bar
    /// </summary>
    /// <param name="LifePointColor"></param>
    public static void setHealtBarColorIndicator(Color LifePointColor)
    {
        ///Set the health bar color base on the remain life that the player have
        _HealthbarImg.color = LifePointColor;
    }

    

}
