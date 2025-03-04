/* ########################################################################

   PICsimLab - PIC laboratory simulator

   ########################################################################

   Copyright (c) : 2019-2020  Luis Claudio Gambôa Lopes

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   For e-mail suggestions :  lcgamboa@yahoo.com
   ######################################################################## */

#include"../picsimlab1.h"
#include"../picsimlab4.h"
#include"../picsimlab5.h"
#include"part_Buzzer.h"

/* outputs */
enum
{
 O_P1, O_P2, O_L1
};

enum
{
 ACTIVE = 0, PASSIVE, TONE
};

cpart_Buzzer::cpart_Buzzer(unsigned x, unsigned y)
: font(9, lxFONTFAMILY_TELETYPE, lxFONTSTYLE_NORMAL, lxFONTWEIGHT_BOLD)
{
 X = x;
 Y = y;
 active = 1;
 aways_update = 1;
 
 ReadMaps ();

 lxImage image (&Window5);
 image.LoadFile (lxGetLocalFile (Window1.GetSharePath () + lxT ("parts/") + GetPictureFileName ()), Orientation, Scale, Scale);

 Bitmap = new lxBitmap (&image, &Window5);
 image.Destroy ();
 canvas.Create (Window5.GetWWidget (), Bitmap);

 input_pins[0] = 0;

 mcount = 0;

 buzzer.Init ();
 type = ACTIVE;

 samplerate = buzzer.GetSampleRate ();
 buffersize = samplerate / 5; //0.1 seconds
 buffer = NULL;
 maxv = buzzer.GetMax ();
 buffercount = 0;
 ctone = 0;
 ftone = 0;
 optone = 0;
 oftone = 0;

 in[0] = 0;
 in[1] = 0;
 in[2] = 0;

 out[0] = 0;
 out[1] = 0;
 out[2] = 0;


}

cpart_Buzzer::~cpart_Buzzer(void)
{
 delete Bitmap;
 if (buffer)
  {
   delete[] buffer;
  }
 canvas.Destroy ();
 buzzer.End ();

}

void
cpart_Buzzer::Draw(void)
{

 int i;

 const picpin * ppins = Window5.GetPinsValues ();

 Update = 0;

 for (i = 0; i < outputc; i++)
  {
   if (output[i].update)//only if need update
    {
     output[i].update = 0;

     if (!Update)
      {
       canvas.Init (Scale, Scale, Orientation);
       canvas.SetFont (font);
      }
     Update++; //set to update buffer

     canvas.SetFgColor (30, 0, 0);

     switch (output[i].id)
      {
      case O_P1:
       canvas.SetColor (49, 61, 99);
       canvas.Rectangle (1, output[i].x1, output[i].y1, output[i].x2 - output[i].x1, output[i].y2 - output[i].y1);
       canvas.SetFgColor (255, 255, 255);
       if (input_pins[output[i].id - O_P1] == 0)
        canvas.RotatedText ("NC", output[i].x1, output[i].y1, 0);
       else
        canvas.RotatedText (Window5.GetPinName (input_pins[output[i].id - O_P1]), output[i].x1, output[i].y1, 0);
       break;
      case O_P2:
       canvas.SetColor (49, 61, 99);
       canvas.Rectangle (1, output[i].x1, output[i].y1, output[i].x2 - output[i].x1, output[i].y2 - output[i].y1);
       canvas.SetFgColor (255, 255, 255);
       canvas.RotatedText ("GND", output[i].x1, output[i].y1, 0);
       break;
      case O_L1:
       canvas.SetFgColor (0, 0, 0);
       color1.Set (55, 0, 0);

       if (input_pins[0] > 0)
        {
         if (active)
          {
           color1.Set (ppins[input_pins[0] - 1].oavalue, 0, 0);
          }
         else
          {
           color1.Set (310 - ppins[input_pins[0] - 1].oavalue, 0, 0);
          }
        }
       //draw a LED
       canvas.SetBgColor (color1);
       int r = color1.Red () - 120;
       int g = color1.Green () - 120;
       int b = color1.Blue () - 120;
       if (r < 0)r = 0;
       if (g < 0)g = 0;
       if (b < 0)b = 0;
       color2.Set (r, g, b);
       canvas.SetBgColor (color2);
       canvas.Circle (1, output[i].x1, output[i].y1, output[i].r + 1);
       canvas.SetBgColor (color1);
       canvas.Circle (1, output[i].x1, output[i].y1, output[i].r - 2);
       break;
      }
    }
  }

 if (Update)
  {
   canvas.End ();
  }
}

unsigned short
cpart_Buzzer::get_in_id(char * name)
{
 printf ("Erro input '%s' don't have a valid id! \n", name);
 return -1;
}

unsigned short
cpart_Buzzer::get_out_id(char * name)
{

 if (strcmp (name, "PN_1") == 0)return O_P1;
 if (strcmp (name, "PN_2") == 0)return O_P2;
 if (strcmp (name, "LD_1") == 0)return O_L1;

 printf ("Erro output '%s' don't have a valid id! \n", name);
 return 1;
};

lxString
cpart_Buzzer::WritePreferences(void)
{
 char prefs[256];

 sprintf (prefs, "%hhu,%hhu,%hhu", input_pins[0], type, active);
 return prefs;
}

void
cpart_Buzzer::ReadPreferences(lxString value)
{
 unsigned char tp;
 sscanf (value.c_str (), "%hhu,%hhu,%hhu", &input_pins[0], &tp, &active);
 ChangeType (tp);
 RegisterRemoteControl ();
}

void
cpart_Buzzer::RegisterRemoteControl(void)
{
 const picpin * ppins = Window5.GetPinsValues ();

 output_ids[O_L1]->status = (void *) &ppins[input_pins[0] - 1].oavalue;

}

void
cpart_Buzzer::ConfigurePropertiesWindow(CPWindow * WProp)
{
 lxString Items = Window5.GetPinsNames ();
 lxString spin;

 ((CCombo*) WProp->GetChildByName ("combo1"))->SetItems (Items);
 if (input_pins[0] == 0)
  ((CCombo*) WProp->GetChildByName ("combo1"))->SetText ("0  NC");
 else
  {
   spin = Window5.GetPinName (input_pins[0]);
   ((CCombo*) WProp->GetChildByName ("combo1"))->SetText (itoa (input_pins[0]) + "  " + spin);
  }

 if (type == ACTIVE)
  ((CCombo*) WProp->GetChildByName ("combo2"))->SetText ("Active");
 else if (type == PASSIVE)
  {
   ((CCombo*) WProp->GetChildByName ("combo2"))->SetText ("Passive");
  }
 else //TONE 
  {
   ((CCombo*) WProp->GetChildByName ("combo2"))->SetText ("Tone");
  }

 if (active)
  ((CCombo*) WProp->GetChildByName ("combo3"))->SetText ("HIGH");
 else
  ((CCombo*) WProp->GetChildByName ("combo3"))->SetText ("LOW ");


 ((CButton*) WProp->GetChildByName ("button1"))->EvMouseButtonRelease = EVMOUSEBUTTONRELEASE & CPWindow5::PropButtonRelease;
 ((CButton*) WProp->GetChildByName ("button1"))->SetTag (1);

 ((CButton*) WProp->GetChildByName ("button2"))->EvMouseButtonRelease = EVMOUSEBUTTONRELEASE & CPWindow5::PropButtonRelease;
}

void
cpart_Buzzer::ReadPropertiesWindow(CPWindow * WProp)
{
 input_pins[0] = atoi (((CCombo*) WProp->GetChildByName ("combo1"))->GetText ());

 unsigned char tp = 0;

 lxString mode = ((CCombo*) WProp->GetChildByName ("combo2"))->GetText ();
 if (mode.compare (lxT ("Active")) == 0)
  {
   tp = ACTIVE;
  }
 else if (mode.compare (lxT ("Passive")) == 0)
  {
   tp = PASSIVE;
  }
 else //TONE
  {
   tp = TONE;
  }

 active = (((CCombo*) WProp->GetChildByName ("combo3"))->GetText ().compare ("HIGH") == 0);

 ChangeType (tp);
 RegisterRemoteControl ();
}

void
cpart_Buzzer::PreProcess(void)
{
 if (type == PASSIVE)
  {
   JUMPSTEPS_ = (Window1.GetBoard ()->MGetInstClockFreq () / samplerate);
   JUMPSTEPS_ *= 100.0 / Window1.timer1.GetTime (); //Adjust to sample at the same time to the timer 
   mcount = JUMPSTEPS_;
  }
 else if (type == TONE)
  {
   JUMPSTEPS_ = (Window1.GetBoard ()->MGetInstClockFreq () / samplerate);
   mcount = JUMPSTEPS_;
   ctone = 0;
   optone = 0;
   ftone = 0;
  }
}

void
cpart_Buzzer::Stop(void)
{
 if ((type == ACTIVE) || (type == TONE))
  {
   buzzer.BeepStop ();
  }
}

void
cpart_Buzzer::Process(void)
{

 if (type == PASSIVE)
  {
   mcount++;
   if (mcount > JUMPSTEPS_)
    {
     if ((input_pins[0])&&(buffercount < buffersize))
      {
       const picpin * ppins = Window5.GetPinsValues ();

       /*       
          0.7837 z-1 - 0.7837 z-2
    y1:  ----------------------
         1 - 1.196 z-1 + 0.2068 z-2
        */
       in[2] = in[1];
       in[1] = in[0];
       if (active)
        {
         in[0] = ((2.0 * ppins[input_pins[0] - 1].value) - 1.0) * maxv * 0.5;
        }
       else
        {
         in[0] = ((2.0 * (ppins[input_pins[0] - 1].value == 0)) - 1.0) * maxv * 0.5;
        }
       out[2] = out[1];
       out[1] = out[0];
       out[0] = 0.7837 * in[1] - 0.7837 * in[2] + 1.196 * out[1] - 0.2068 * out[2];

       buffer[buffercount++] = out[0];

      }

     mcount = 0;
    }
  }
 else if (type == TONE)
  {
   mcount++;
   if (mcount > JUMPSTEPS_)
    {
     if (input_pins[0])
      {
       const picpin * ppins = Window5.GetPinsValues ();

       if ((!optone)&&(ppins[input_pins[0] - 1].value))
        {
         ftone = (ftone + ctone) / 2.0;
         ctone = 0;
        }
       optone = ppins[input_pins[0] - 1].value;
       ctone++;
      }
     mcount = 0;
    }
  }
}

void
cpart_Buzzer::PostProcess(void)
{
 const picpin * ppins = Window5.GetPinsValues ();

 if (type == ACTIVE)
  {
   if (active)
    {
     if (ppins[input_pins[0] - 1].oavalue > 65)
      {
       buzzer.BeepStart ();
      }
     else
      {
       buzzer.BeepStop ();
      }
    }
   else
    {
     if ((310 - ppins[input_pins[0] - 1].oavalue) > 215)
      {
       buzzer.BeepStart ();
      }
     else
      {
       buzzer.BeepStop ();
      }
    }
  }
 else if (type == PASSIVE)
  {

   //int ret=
   buzzer.SoundPlay (buffer, buffercount);
   //printf("ret=%i buffercount=%i sample=%i time=%f  timer=%i\n",ret,buffercount,samplerate,((float)(buffercount))/samplerate , Window1.timer1.GetTime ());
   buffercount = 0;
  }
 else //TONE
  {

   float freq;

   if (ftone > 5)
    {
     freq = samplerate / ftone;
    }
   else
    {
     freq = 0;
    }

   if (freq > 100)
    {
     if (fabs (oftone - freq) > 10.0)
      {
       buzzer.BeepStop ();
       buzzer.BeepStart (freq, 0.5, 1);
       oftone = freq;
      }
    }
   else
    {
     buzzer.BeepStop ();
     oftone = 0;
    }

  }

 if (input_pins[0] && (output_ids[O_L1]->value != ppins[input_pins[0] - 1].oavalue))
  {
   output_ids[O_L1 ]->value = ppins[input_pins[0] - 1].oavalue;
   output_ids[O_L1 ]->update = 1;
  }

}

void
cpart_Buzzer::ChangeType(unsigned char tp)
{
 if (type == tp) return;

 if ((type == ACTIVE) || (type == TONE))
  {
   buzzer.BeepStop ();
   if (!buffer)
    {
     buffer = new short[buffersize];
    }
   type = tp;
  }
 else if (type == PASSIVE)
  {
   if (buffer)
    {
     delete[] buffer;
     buffer = NULL;
    }
   type = tp;
  }
}

part_init(PART_BUZZER_Name, cpart_Buzzer, "Output");

