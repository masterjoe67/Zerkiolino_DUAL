library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity neorv32_ps2_wb is
  port (
    -- Wishbone
    clk_i    : in  std_logic;
    rst_i    : in  std_logic;
    stb_i    : in  std_logic;
    cyc_i    : in  std_logic;
    we_i     : in  std_logic;
    addr_i   : in  std_logic_vector(2 downto 0); 
    dat_i    : in  std_logic_vector(31 downto 0);
    dat_o    : out std_logic_vector(31 downto 0);
    ack_o    : out std_logic;
    
    -- PS/2
    ps2_clk  : in  std_logic;
    ps2_data : in  std_logic
  );
end neorv32_ps2_wb;

architecture rtl of neorv32_ps2_wb is
  -- Segnali per il filtro digitale (Debounce)
  signal clk_filter  : std_logic_vector(7 downto 0) := (others => '1');
  signal clk_clean   : std_logic := '1';
  signal clk_prev    : std_logic := '1';
  signal data_sync   : std_logic_vector(1 downto 0) := (others => '1');

  -- Ricevitore fisico
  signal bit_cnt      : integer range 0 to 10 := 0;
  signal shift_reg    : std_logic_vector(10 downto 0) := (others => '0');
  
  -- FIFO (16 livelli)
  type fifo_t is array (0 to 15) of std_logic_vector(7 downto 0);
  signal fifo_mem : fifo_t := (others => (others => '0'));
  signal wr_ptr, rd_ptr : unsigned(3 downto 0) := (others => '0');
  signal fifo_empty : std_logic;

  signal ack_internal : std_logic := '0';

begin

  -- 1) Filtro Digitale e Sincronizzazione
  -- PS/2 corre a ~10kHz, il sistema a 60MHz. Dobbiamo ripulire il clock.
  process(clk_i)
  begin
    if rising_edge(clk_i) then
      -- Filtro per il clock
      clk_filter <= clk_filter(6 downto 0) & ps2_clk;
      if clk_filter = "11111111" then
        clk_clean <= '1';
      elsif clk_filter = "00000000" then
        clk_clean <= '0';
      end if;
      
      clk_prev  <= clk_clean;
      
      -- Sincronizzazione dati per evitare metastabilità
      data_sync <= data_sync(0) & ps2_data;
    end if;
  end process;

-- 2) Cattura Scancode (Modificata per allineamento LSB-first)
  process(clk_i, rst_i)
  begin
    if rst_i = '1' then
      bit_cnt <= 0;
      wr_ptr <= (others => '0');
      shift_reg <= (others => '0');
    elsif rising_edge(clk_i) then
      if clk_prev = '1' and clk_clean = '0' then
        -- Carichiamo i bit dall'alto verso il basso (MSB -> LSB) 
        -- perché lo shift_reg(10 downto 1) con data_sync in testa 
        -- inverte l'ordine se non stiamo attenti.
        shift_reg <= data_sync(1) & shift_reg(10 downto 1);
        
        if bit_cnt = 10 then
          -- Proviamo l'estrazione "specchiata" che corregge l'ordine LSB
          -- Se 217 diventasse 0x66, abbiamo vinto.
          fifo_mem(to_integer(wr_ptr)) <= shift_reg(9 downto 2); 
          
          wr_ptr <= wr_ptr + 1;
          bit_cnt <= 0;
        else
          bit_cnt <= bit_cnt + 1;
        end if;
      end if;
    end if;
  end process;

  fifo_empty <= '1' when (wr_ptr = rd_ptr) else '0';

  -- 3) Interfaccia Wishbone Slave
  process(clk_i)
  begin
    if rising_edge(clk_i) then
      if rst_i = '1' then
        ack_internal <= '0';
        rd_ptr <= (others => '0');
        dat_o <= (others => '0');
      else
        ack_internal <= '0';
        if (cyc_i = '1' and stb_i = '1' and ack_internal = '0') then
          ack_internal <= '1';
          dat_o <= (others => '0');
          
          if we_i = '0' then -- Lettura
            if addr_i(2) = '0' then -- DATA (offset 0)
              dat_o(7 downto 0) <= fifo_mem(to_integer(rd_ptr));
              if fifo_empty = '0' then
                rd_ptr <= rd_ptr + 1;
              end if;
            else -- STATUS (offset 4)
              dat_o(0) <= not fifo_empty;
            end if;
          end if;
        end if;
      end if;
    end if;
  end process;

  ack_o <= ack_internal;

end rtl;