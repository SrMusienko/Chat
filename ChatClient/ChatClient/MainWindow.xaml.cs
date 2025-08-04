using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Windows;
using System.Windows.Controls;

namespace ChatClient
{
    public partial class MainWindow : Window
    {
        private TcpClient client;
        private NetworkStream stream;
        private Thread listenThread;
        public ObservableCollection<TodoItem> TodoItems { get; set; } = new ObservableCollection<TodoItem>();

        public MainWindow()
        {
            InitializeComponent();
            TodoListView.ItemsSource = TodoItems;

            try
            {
                client = new TcpClient("127.0.0.1", 1111);
                stream = client.GetStream();

                listenThread = new Thread(ListenToServer) { IsBackground = true };
                listenThread.Start();

                Send("GET"); // загрузка задач
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Ошибка подключения: {ex.Message}");
            }
        }

        private void AddButton_Click(object sender, RoutedEventArgs e)
        {
            var text = DescriptionBox.Text.Trim();
            if (!string.IsNullOrWhiteSpace(text))
            {
                Send($"ADD {text}");
                DescriptionBox.Clear();
            }
        }

        private void ToggleStatus(object sender, RoutedEventArgs e)
        {
            if (sender is CheckBox cb && cb.DataContext is TodoItem item)
            {
                Send($"TOGGLE {item.Id}");
            }
        }

        private void ListenToServer()
        {
            var buffer = new byte[4096];
            while (true)
            {
                try
                {
                    int bytes = stream.Read(buffer, 0, buffer.Length);
                    if (bytes <= 0) break;

                    string message = Encoding.UTF8.GetString(buffer, 0, bytes);
                    ParseAndUpdateList(message);
                }
                catch
                {
                    break;
                }
            }
        }

        private void ParseAndUpdateList(string data)
        {
            var lines = data.Split(new[] { '\n', '\r' }, StringSplitOptions.RemoveEmptyEntries);

            Application.Current.Dispatcher.Invoke(() => {
                TodoItems.Clear();
                foreach (var line in lines)
                {
                    var parts = line.Split('|');
                    if (parts.Length == 3 &&
                        int.TryParse(parts[0].Trim(), out int id))
                    {
                        TodoItems.Add(new TodoItem
                        {
                            Id = id,
                            Description = parts[1].Trim(),
                            Completed = parts[2].Trim().Equals("Completed", StringComparison.OrdinalIgnoreCase)
                        });
                    }
                }
            });
        }

        private void Send(string message)
        {
            if (stream != null && stream.CanWrite)
            {
                byte[] buffer = Encoding.UTF8.GetBytes(message);
                stream.Write(buffer, 0, buffer.Length);
            }
        }
    }

    public class TodoItem : INotifyPropertyChanged
    {
        public int Id { get; set; }
        public string Description { get; set; }

        private bool _completed;
        public bool Completed
        {
            get => _completed;
            set
            {
                if (_completed != value)
                {
                    _completed = value;
                    PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(Completed)));
                }
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;
    }
}
