using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.SceneManagement;

public class TimeSystem : MonoBehaviour
{
    public string EndGameScene;

    public Text clockText;
    public Text dayText;
    public Text yearText;
    public Text monthText;

    public static double minute, hour, day, second, month, year;

    private const int TIMESCALE = 60;

    public bool _CountdownTimer = true;
   

    // Start is called before the first frame update
    void Start()
    {
       
    }

    // Update is called once per frame
    void Update()
    {
        MaxDate();
        if(_CountdownTimer == true)
        {
            CalculateTime();
        }
        
    }

    void TextCallFunction()
    {
        dayText.text = "" + day;
        clockText.text = string.Format("{0:00}:{1:00}", hour, minute);
        yearText.text = "" + year;
        monthText.text = "" + month;
    }

    void CalculateMonth()
    {
        if (month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12)
        {
            if (day >= 32)
            {
                month++;
                day = 1;
                TextCallFunction();
            }
        }

        if (month == 4 || month == 6 || month == 9 || month == 11)
        {
            if (day >= 31)
            {
                month++;
                day = 1;
                TextCallFunction();
            }
        }

        if (month == 2)
        {
            month++;
            day = 1;
            TextCallFunction();
        }


    }
    void CalculateTime()
    {
        second += Time.deltaTime * TIMESCALE;

        if (second > 60)
        {
            minute++;
            second = 0;
            TextCallFunction();
        }
        else if (minute >= 60)
        {
            hour++;
            minute = 0;
            TextCallFunction();
        }
        else if (hour >= 24)
        {
            day++;
            hour = 7;
            TextCallFunction();
        }
        else if (day >= 28)
        {
            CalculateMonth();
        }
        else if (month > 12)
        {
            month = 1;
            year++;
            TextCallFunction();

        }
    }


    void MaxDate()
    {
        if(day == 7)
        {
            _CountdownTimer = false;
            dayText.text = "-";
            yearText.text = "-";
            monthText.text = "-";
            SceneManager.LoadScene(EndGameScene);
        }
    }
}

