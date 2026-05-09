-- ****************************************************************************
-- * Wishbone Interconnect: SDRAM (0x9...), VGA (0x4...), Keyboard (0x5...)
-- ****************************************************************************

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity wb_intercon is
   port (  
      -- Syscon
      clk_i      : in  std_logic := '0';
      rst_i      : in  std_logic := '0';
      
      -- Wishbone Master (NEORV32 XBUS)
      wbm_stb_i  : in  std_logic := '0';
      wbm_cyc_i  : in  std_logic := '0';
      wbm_we_i   : in  std_logic := '0';
      wbm_ack_o  : out std_logic;
      wbm_adr_i  : in  std_logic_vector(31 downto 0) := (others => '0');
      wbm_dat_i  : in  std_logic_vector(31 downto 0) := (others => '0');
      wbm_dat_o  : out std_logic_vector(31 downto 0);
      wbm_sel_i  : in  std_logic_vector(03 downto 0) := (others => '0');
      
      -- Wishbone Slave x (Segnali comuni)
      wbs_we_o   : out std_logic; 
      wbs_dat_o  : out std_logic_vector(31 downto 0);  
      wbs_sel_o  : out std_logic_vector(03 downto 0);  
      
      -- Wishbone Slave 1 (SDRAM - Range 0x90000000)
      wbs1_stb_o : out std_logic;
      wbs1_cyc_o : out std_logic;
      wbs1_ack_i : in  std_logic := '0';
      wbs1_adr_o : out std_logic_vector(27 downto 0);  
      wbs1_dat_i : in  std_logic_vector(31 downto 0) := (others => '0');

      -- Wishbone Slave 2 (VGA - Range 0x40000000)
      wbs2_stb_o : out std_logic;
      wbs2_cyc_o : out std_logic;
      wbs2_ack_i : in  std_logic := '0';
      wbs2_adr_o : out std_logic_vector(23 downto 0);  
      wbs2_dat_i : in  std_logic_vector(31 downto 0) := (others => '0');

      -- Wishbone Slave 3 (KEYBOARD - Range 0x50000000)
      wbs3_stb_o : out std_logic;
      wbs3_cyc_o : out std_logic;
      wbs3_ack_i : in  std_logic := '0';
      wbs3_adr_o : out std_logic_vector(23 downto 0); -- Bastano pochi bit
      wbs3_dat_i : in  std_logic_vector(31 downto 0) := (others => '0')
   );
end entity wb_intercon;

architecture syn of wb_intercon is
   signal wbs1_enable : std_logic := '0';
   signal wbs2_enable : std_logic := '0';
   signal wbs3_enable : std_logic := '0';
begin
   
   -- Decoder: seleziona basandosi sui primi 4 bit (Nibble alto)
   wbs1_enable <= '1' when (wbm_adr_i(31 downto 28) = x"9") else '0'; -- SDRAM
   wbs2_enable <= '1' when (wbm_adr_i(31 downto 28) = x"4") else '0'; -- VGA
   wbs3_enable <= '1' when (wbm_adr_i(31 downto 28) = x"5") else '0'; -- KEYBOARD
   
   -- Pass-through degli indirizzi locali
   wbs1_adr_o  <= wbm_adr_i(27 downto 0);
   wbs2_adr_o  <= wbm_adr_i(23 downto 0);
   wbs3_adr_o  <= wbm_adr_i(23 downto 0);

   -- Master to Slave (Controllo)
   wbs_we_o    <= wbm_we_i;
   wbs_dat_o   <= wbm_dat_i; 
   wbs_sel_o   <= wbm_sel_i; 
   
   wbs1_stb_o  <= wbs1_enable and wbm_stb_i;
   wbs1_cyc_o  <= wbs1_enable and wbm_cyc_i;

   wbs2_stb_o  <= wbs2_enable and wbm_stb_i;
   wbs2_cyc_o  <= wbs2_enable and wbm_cyc_i;

   wbs3_stb_o  <= wbs3_enable and wbm_stb_i;
   wbs3_cyc_o  <= wbs3_enable and wbm_cyc_i;

   -- Slave to Master (Multiplexer Risposte)
   wbm_ack_o <= wbs1_ack_i when (wbs1_enable = '1') else
                wbs2_ack_i when (wbs2_enable = '1') else
                wbs3_ack_i when (wbs3_enable = '1') else
                '0';
   
   wbm_dat_o <= wbs1_dat_i when (wbs1_enable = '1') else
                wbs2_dat_i when (wbs2_enable = '1') else
                wbs3_dat_i when (wbs3_enable = '1') else
                (others => '0');

end architecture syn;