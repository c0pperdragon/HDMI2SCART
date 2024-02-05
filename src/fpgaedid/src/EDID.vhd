library ieee;
use ieee.numeric_std.all;
use ieee.std_logic_1164.all;

entity EDID is	
	port (
		SDA: in std_logic;
		SCL: in std_logic;
		SDA_PULLDOWN: out std_logic
	);	
end entity;


architecture immediate of EDID is

COMPONENT OSCH
	GENERIC (NOM_FREQ: string);
	PORT (
		STDBY:IN std_logic;
		OSC:OUT std_logic;
		SEDSTDBY:OUT std_logic
	);
END COMPONENT;

signal OSC : std_logic;

begin
	OSCInst0: OSCH
	GENERIC MAP( NOM_FREQ => "20.46" )
	PORT MAP ( STDBY=> '0', OSC => OSC, SEDSTDBY => open );

	process (SDA,SCL,OSC) 
	
	constant PIXELCLOCK:integer := 2*1348; -- 14625;
	constant HACTIVE:integer := 2*720; 
	constant HBLANK:integer := 2*144; 	
	constant HFP:integer := 2*12; 
	constant HSYNC:integer := 2*64; 
	constant VACTIVE:integer := 288;
	constant VBLANK:integer := 24; 
	constant VFP:integer := 3; 
	constant VSYNC:integer := 3; 
	constant HSIZE:integer:=400;
	constant VSIZE:integer:=300;
	
	-- experimental EDID for HDMI2SCART
	type T_edid is array (0 to 127) of integer range 0 to 255;
    constant edid : T_edid := (
  0,16#FF#,16#FF#,16#FF#,16#FF#,16#FF#,16#FF#,0, -- 0-7 fixed header
  16#0D#, 16#F0#,                           -- 8-9 manufacturer: "COP"
  1, 0,                                     -- 10-11 product code: 1
  1, 0, 0, 0,                               -- 12-15 serial number: 1
  10, 34,                                   -- 16-17 week and year (10,2024)
  1, 3,                                     -- 18-19 EDID version
  2#10100010#,                              -- 20 digital video parameters
  HSIZE/10,                                 -- 21 screen width in centimeters
  VSIZE/10,                                 -- 22 screen height in centimeters
  120,                                      -- 23 display gamma value
  2#00100110#,                              -- 24 feature bitmap (RGB color)
  16#B7#,16#F5#,16#A8#,16#54#,16#35#,16#AD#,16#25#,16#10#,16#50#,16#54#,  -- 25-34 chromaticity coordinates
  16#00#,16#00#,16#00#,                     -- 35-37 established timings bitmap (unused)
  16#01#,16#01#,                            -- 38-39 standard timing 1 (unused)
  16#01#,16#01#,                            -- 40-41 standard timing 2 (unused)
  16#01#,16#01#,                            -- 42-43 standard timing 3 (unused)
  16#01#,16#01#,                            -- 44-45 standard timing 4 (unused)
  16#01#,16#01#,                            -- 46-47 standard timing 5 (unused)
  16#01#,16#01#,                            -- 48-49 standard timing 6 (unused)
  16#01#,16#01#,                            -- 50-51 standard timing 7 (unused)
  16#01#,16#01#,                            -- 52-53 standard timing 8 (unused)
                                            -- 54-71 detailed timing 1
  PIXELCLOCK mod 256, PIXELCLOCK / 256,   --   0-1 pixel clock
  HACTIVE mod 256,                         --   2 horizontal active pixels 8 lsb    
  HBLANK mod 256,                          --   3 horizontal blanking pixels 8 lsb    
  (HACTIVE/256)*16+HBLANK/256,            --   4 hor act & blanking 4 msb each
  VACTIVE mod 256,                         --   5 vertical active lines 8 lsb
  VBLANK mod 256,                          --   6 vertical blanking lines 8 lsb
  (VACTIVE/256)*16+VBLANK/256,            --   7 ver act & blanking 4 msb each
  HFP,                                     --   8 horizontal front porch 8 lsb   
  HSYNC,                                   --   9 horizontal sync pulse width 8 lsb
  VFP*16 + VSYNC,                          --   10 vert front porch & sync 4 lsb each
  16#00#,                                  --   11 hor fp, hor sync, vert fp, vert sync, 2 msb each
  HSIZE mod 256,                          --   12 horizontal image size (mm), 8 lsb
  VSIZE mod 256,                          --   13 vertical image size (mm), 8 lsb
  (HSIZE/256)*16+VSIZE/256,               --   14 hor & vert image size 4msb each
  16#00#,                                  --   15 horizontal border
  16#00#,                                  --   16 vertical border
  2#00011000#,                             --   17 features bitmap (separate sync, negative polarity) 
                                           --  72-89 Monitor descriptor
      16#00#,16#00#,16#00#,16#FC#,        --     0-3  specify monitor name
	  0,                                   --     4   reserved
	  72,68,77,73,50,83,67,65,82,84,10,  --     5-15 'HDMI2SCART\n'
	  32,32,                               --     16-17 padding
	                                       -- 90-107 dummy descriptor
  	  16#00#,16#00#,16#00#,16#10#,16#00#,16#00#,16#00#,16#00#,16#00#,16#00#,16#00#,16#00#,16#00#,16#00#,16#00#,16#00#,16#00#,16#00#,
	                                       -- 108-125 dummy descriptor
  	  16#00#,16#00#,16#00#,16#10#,16#00#,16#00#,16#00#,16#00#,16#00#,16#00#,16#00#,16#00#,16#00#,16#00#,16#00#,16#00#,16#00#,16#00#,
      0,                                   -- 126 number of extension blocks
      0                                    -- 127 checksum 1
	);
	
	variable in_sda:std_logic := '1';
	variable in_scl:std_logic := '1';
	variable prev_sda:std_logic := '1';
	variable prev_scl:std_logic := '1';
	
	variable started:boolean := false;
	variable bitcounter:integer range 31 downto 0;
	variable receivebuffer:std_logic_vector(7 downto 0);
	variable writing:boolean;
	variable reading:boolean;
	variable registeraddress:integer range 0 to 255 := 0;

	variable tmp8:std_logic_vector(7 downto 0);
	variable checksum:integer range 0 to 255;
	begin
		checksum := 0;
		for i in 0 to 126 loop 
			checksum := (checksum + edid(i)) mod 256;
		end loop;
		checksum := (256 - checksum) mod 256;
	
		if rising_edge(OSC) then
			-- SDA and SCL changed simultaneosly
			if in_sda/=prev_sda and in_scl/=prev_scl then   
				started := false;
				SDA_PULLDOWN <= '0';
			-- start signal
			elsif prev_scl='1' and in_scl='1' and prev_sda='1' and in_sda='0' then 
				started := true;
				bitcounter := 0;
				writing := false;
				reading := false;
				SDA_PULLDOWN <= '0';
			-- stop signal
			elsif prev_scl='1' and in_scl='1' and prev_sda='0' and in_sda='1' then 
				started := false;
				SDA_PULLDOWN <= '0';
			-- at falling edge of SCL, data is transfered in either direction
			elsif started and prev_scl='1' and in_scl='0' then
				tmp8 := std_logic_vector(to_unsigned(edid(registeraddress),8));
				if registeraddress=127 then tmp8 := std_logic_vector(to_unsigned(checksum,8)); end if;
				if registeraddress>=128 then tmp8 := "11111111"; end if;
				-- check address 
				if bitcounter=8 and receivebuffer(6 downto 0)="1010000" then
					SDA_PULLDOWN <= '1'; -- ACK the address
					if in_sda='0' then  -- set reading/writing
						writing:=true;
						reading:=false;
					else
						writing:=false;
						reading:=true;
					end if;
				-- ACK data byte
				elsif writing and bitcounter=17 then
					SDA_PULLDOWN <= '1';  
				-- receive register address from master
				elsif writing and bitcounter=18 then
					registeraddress := to_integer(signed(receivebuffer));
					SDA_PULLDOWN <= '0';
				-- transfer data to master
				elsif reading and bitcounter>=9 and bitcounter<=16 then
					SDA_PULLDOWN <= not tmp8(16-bitcounter);
				-- transfer first bit of subsequence byte (only after master did ACK)
				elsif reading and bitcounter=18 and in_sda='0' then
					SDA_PULLDOWN <= not tmp8(7);
				-- default if nothing else applies
				else
					SDA_PULLDOWN <= '0'; 
				end if;
				-- progress register address
				if bitcounter=17 then
					registeraddress := (registeraddress+1) mod 256;
				end if;
				-- receive incomming bit
				receivebuffer := receivebuffer(6 downto 0) & in_sda;
				-- progress bit counter
				if bitcounter<18 then
					bitcounter := bitcounter+1;
				else
					bitcounter := 10;
				end if;
			end if;
			
			prev_sda := in_sda;
			prev_scl := in_scl;
			in_sda := SDA;
			in_scl := SCL;			
		end if;
	end process;	
end immediate;
